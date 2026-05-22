#include "filestream.hpp"
#include "khiops_driver_common/util.hpp"
#include "khiops_driver_common/logging.hpp"
#include <azure/storage/blobs/block_blob_client.hpp>
#include <azure/storage/common/storage_exception.hpp>
#include <chrono>
#include <iomanip>
#include <spdlog/spdlog.h>
#include <sstream>

using namespace std;
using namespace khiops_driver_common;

namespace khiops_driver_azure {
int FileStream::OpenForReading(
    FileStream *result,
    const std::vector<Azure::Storage::Blobs::BlobClient> &clients) {
  return OpenForReading(result,
                        vector<ObjectClient>(clients.begin(), clients.end()));
}

int FileStream::OpenForReading(
    FileStream *result,
    const std::vector<Azure::Storage::Files::Shares::ShareFileClient>
        &clients) {
  return OpenForReading(result,
                        vector<ObjectClient>(clients.begin(), clients.end()));
}

int FileStream::OpenForReading(FileStream *result,
                               const std::vector<ObjectClient> &clients) {
  if (clients.empty()) {
    GetLogger()->error("No object client found.");
    return -1;
  }
  FileStream fs;
  fs.storageType = clients.front().tag;
  fs.mode = Mode::READ;
  fs.readInfo = make_unique<FragmentedFile>(clients);
  *result = std::move(fs);
  return 0;
}

void FileStream::OpenForWriting(
    FileStream *result, OutputMode mode,
    const Azure::Storage::Blobs::BlobClient &client) {
  OpenForWriting(result, mode, ObjectClient(client));
}

void FileStream::OpenForWriting(
    FileStream *result, OutputMode mode,
    const Azure::Storage::Files::Shares::ShareFileClient &client) {
  OpenForWriting(result, mode, ObjectClient(client));
}

void FileStream::OpenForWriting(FileStream *result, OutputMode mode,
                                const ObjectClient &client) {
  FileStream fs;
  fs.storageType = client.tag;
  fs.mode = Mode::WRITE;
  fs.writeInfo = make_unique<WriteInfo>(mode, client, vector<string>());

  if (fs.storageType == BLOB) {
    if (fs.writeInfo->mode == OutputMode::APPEND) {
      try {
        vector<Azure::Storage::Blobs::Models::BlobBlock> blocks;
        auto blockListRequestResponse =
            fs.writeInfo->client.blob.AsBlockBlobClient().GetBlockList();
        blocks = blockListRequestResponse.Value.CommittedBlocks;
        transform(blocks.begin(), blocks.end(),
                  back_inserter(fs.writeInfo->blockIds),
                  [](const auto &block) { return block.Name; });
      } catch (const Azure::Storage::StorageException &) {
      }
    }
  } else // SHARE storage
  {
    if (fs.writeInfo->mode == OutputMode::WRITE)
      fs.writeInfo->client.shareFile.Create(0);
    else // APPEND mode
      fs.nCurrentPos =
          (size_t)fs.writeInfo->client.shareFile.GetProperties().Value.FileSize;
  }
  *result = std::move(fs);
}

FileStream::FileStream()
    : handle((void *)chrono::steady_clock::now().time_since_epoch().count()),
      nCurrentPos(0ULL),
      readInfo(nullptr),
      writeInfo(nullptr) {}

FileStream::FileStream(FileStream &&source)
  : handle(std::move(source.handle)),
    storageType(std::move(source.storageType)), mode(std::move(source.mode)),
    nCurrentPos(std::move(source.nCurrentPos)),
    readInfo(std::move(source.readInfo)),
    writeInfo(std::move(source.writeInfo)) {
}

FileStream &FileStream::operator=(FileStream &&other) {
  if (this == &other) return *this;
  handle = std::move(other.handle);
  storageType = std::move(other.storageType);
  mode = std::move(other.mode);
  nCurrentPos = std::move(other.nCurrentPos);
  readInfo = std::move(other.readInfo);
  writeInfo = std::move(other.writeInfo);
  return *this;
}

void *FileStream::GetHandle() const { return handle; }

FileStream::Mode FileStream::GetMode() const { return mode; }

FileStream::WriteInfo::WriteInfo(OutputMode mode, const ObjectClient &client,
                                 const std::vector<std::string> &blockIds)
    : mode(mode), client(client), blockIds(blockIds) {}

FileStream::WriteInfo::WriteInfo(WriteInfo &&source)
    : mode(std::move(source.mode)), client(std::move(source.client)),
      blockIds(std::move(source.blockIds)) {}

FileStream::WriteInfo::~WriteInfo() { blockIds.clear(); }

void FileStream::Close() {
  if (mode == Mode::WRITE)
    Flush();
}

int FileStream::Read(size_t *nRead, void *dest, size_t nSize, size_t nCount) {
  if (mode != Mode::READ) {
    GetLogger()->error("Operation 'read' is invalid for stream mode.");
    return -1;
  }

  size_t nTotalFileSize = readInfo->GetSize();
  size_t nToRead = nSize * nCount;
  size_t nRead_ = 0;
  size_t nTotalRead = 0;
  size_t nFragmentIndex;
  int nStatus;
  if ((nStatus = readInfo->GetFragmentIndexOfUserOffset(&nFragmentIndex,
                                                       nCurrentPos))) {
    return nStatus;
  }

  while (nToRead != 0) {
    const FragmentedFile::Fragment &fragment =
        readInfo->GetFragment(nFragmentIndex);

    Azure::Core::Http::HttpRange range{
        (int64_t)((nFragmentIndex == 0 ? 0 : readInfo->GetHeaderLen()) +
                  nCurrentPos - fragment.nUserOffset),
        (int64_t)(nToRead < fragment.nContentSize ? nToRead
                                                  : fragment.nContentSize)};

    unique_ptr<Azure::Core::IO::BodyStream> bodyStream;

    try {
      if (fragment.client.tag == BLOB) {
        Azure::Storage::Blobs::BlobAccessConditions accessConditions;
        accessConditions.IfMatch = fragment.etag;
        Azure::Storage::Blobs::DownloadBlobOptions opts;
        opts.AccessConditions = accessConditions;
        opts.Range = range;
        auto downloadResult =
            std::move(fragment.client.blob.Download(opts).Value);
        bodyStream = std::move(downloadResult.BodyStream);
      } else // SHARE storage
      {
        Azure::Storage::Files::Shares::DownloadFileOptions opts;
        opts.Range = range;
        auto downloadResult =
            std::move(fragment.client.shareFile.Download(opts).Value);
        if (downloadResult.Details.ETag != fragment.etag) {
          GetLogger()->error("The file has been updated while reading it.");
          return -1;
        }
        bodyStream = std::move(downloadResult.BodyStream);
      }
    } catch (const Azure::Storage::StorageException &exc) {
      if (exc.StatusCode ==
          Azure::Core::Http::HttpStatusCode::PreconditionFailed) {
        GetLogger()->error("The file has been updated while reading it.");
        return -1;
      }
      if (exc.StatusCode ==
          Azure::Core::Http::HttpStatusCode::RangeNotSatisfiable) {
        GetLogger()->error("Cannot read after end of file.");
        *nRead = 0;
        return -2;
      }
      throw;
    }
    nRead_ = bodyStream->ReadToCount((uint8_t *)dest, nToRead);

    if (nToRead > 0 && nRead_ == 0) {
      // Handle emulator special behavior that gracefully
      // accepts read beyond file size
      GetLogger()->error("Cannot read after end of file.");
      *nRead = 0;
      return -2;
    }

    nToRead -= nRead_;
    nTotalRead += nRead_;
    nCurrentPos += nRead_;
    dest = (uint8_t *)dest + nRead_;
    if (nCurrentPos == nTotalFileSize)
      break;
    nFragmentIndex++;
  }
  *nRead = nTotalRead;
  return 0;
}

int FileStream::Seek(long long int nOffset, int nOrigin) {
  if (mode != Mode::READ) {
    GetLogger()->error("Operation 'seek' is invalid for stream mode.");
    return -1;
  }

  size_t nTotalFileSize = readInfo->GetSize();
  long long int nSignedDest;

  switch (nOrigin) {
  case ios::beg:
    nSignedDest = nOffset;
    break;
  case ios::cur:
    nSignedDest = (long long int)nCurrentPos + nOffset;
    break;
  case ios::end:
    nSignedDest = (long long int)nTotalFileSize + nOffset;
    break;
  default:
    GetLogger()->error("Invalid seek origin {}.", nOrigin);
    return -1;
  }

  if (nSignedDest < 0 || nSignedDest >= (long long int)nTotalFileSize) {
    GetLogger()->error("Invalid seek offset {} for origin {}.", nOffset,
                       nOrigin);
    return -1;
  }

  nCurrentPos = (size_t)nSignedDest;

  return 0;
}

int FileStream::Write(size_t *nWritten, const void *source, size_t nSize,
                      size_t nCount) {
  if (mode != Mode::WRITE) {
    GetLogger()->error("Operation 'write' is invalid for stream mode.");
    return -1;
  }

  size_t nToWrite = nSize * nCount;

  if (storageType == BLOB) {
    Azure::Storage::Blobs::BlockBlobClient bbclient =
        writeInfo->client.blob.AsBlockBlobClient();

    ostringstream oss;
    oss << setfill('0') << setw(64) << writeInfo->blockIds.size();
    string sBlockIdInBase10 = oss.str();
    vector<uint8_t> blockIdInBase10(sBlockIdInBase10.begin(),
                                    sBlockIdInBase10.end());
    string sBlockIdInBase64 =
        Azure::Core::Convert::Base64Encode(blockIdInBase10);

    Azure::Core::IO::MemoryBodyStream bodyStream((const uint8_t *)source,
                                                 nToWrite);
    bbclient.StageBlock(sBlockIdInBase64, bodyStream);
    writeInfo->blockIds.push_back(sBlockIdInBase64);
  } else // SHARE storage
  {
    Azure::Storage::Files::Shares::Models::FileHttpHeaders httpHeaders;
    Azure::Storage::Files::Shares::Models::FileSmbProperties smbProperties;
    Azure::Storage::Files::Shares::SetFilePropertiesOptions opts;

    Azure::Core::IO::MemoryBodyStream bodyStream((const uint8_t *)source,
                                                 nToWrite);
    opts.Size = nCurrentPos + nToWrite;
    writeInfo->client.shareFile.SetProperties(httpHeaders, smbProperties, opts);
    writeInfo->client.shareFile.UploadRange((int64_t)nCurrentPos, bodyStream);
    nCurrentPos += nToWrite;
  }

  *nWritten = nToWrite;
  return 0;
}

int FileStream::Flush() {
  if (mode != Mode::WRITE) {
    GetLogger()->error("Operation 'flush' is invalid for stream mode.");
    return -1;
  }

  if (storageType == BLOB) {
    writeInfo->client.blob.AsBlockBlobClient().CommitBlockList(
        writeInfo->blockIds);
  } else {
    writeInfo->client.shareFile.ForceCloseAllHandles();
  }

  return 0;
}
} // namespace khiops_driver_azure
