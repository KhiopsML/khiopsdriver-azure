#include "driver.hpp"
#include "auth.hpp"
#include "blobpathresolve.hpp"
#include "connstr.hpp"
#include "khiops_driver_common/util.hpp"
#include "khiops_driver_common/logging.hpp"
#include "servicerequest.hpp"
#include "sharepathresolve.hpp"
#include "storagetype.hpp"
#include <algorithm>
#include <azure/core.hpp>
#include <azure/core/diagnostics/logger.hpp>
#include <azure/core/http/curl_transport.hpp>
#include <azure/identity.hpp>
#include <azure/storage/blobs/blob_options.hpp>
#include <azure/storage/blobs/block_blob_client.hpp>
#include <azure/storage/files/shares/share_options.hpp>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <regex>
#include <spdlog/spdlog.h>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

using namespace std;
using namespace khiops_driver_common::util;
using khiops_driver_common::logging::getLogger;

#if defined(__linux__)
static Azure::Core::Http::Policies::TransportOptions MakeTransportOptions() {
  Azure::Core::Http::CurlTransportOptions curl_transport_options;
  FindCertificate(&curl_transport_options.CAInfo);
  Azure::Core::Http::Policies::TransportOptions transport_options;
  transport_options.Transport = make_shared<Azure::Core::Http::CurlTransport>(curl_transport_options);
  return transport_options;
}
#endif

static Azure::Storage::Blobs::BlobClientOptions MakeBlobClientOptions() {
  Azure::Storage::Blobs::BlobClientOptions blob_client_options;
#if defined(__linux__)
  blob_client_options.Transport = MakeTransportOptions();
#endif
  return blob_client_options;
}

static Azure::Storage::Files::Shares::ShareClientOptions MakeShareClientOptions() {
  Azure::Storage::Files::Shares::ShareClientOptions share_client_options;
#if defined(__linux__)
  share_client_options.Transport = MakeTransportOptions();
#endif
  share_client_options.ShareTokenIntent = Azure::Storage::Files::Shares::Models::ShareTokenIntent::Backup;
  return share_client_options;
}

namespace az {

Driver::Driver(size_t nPreferredBufferSize)
    : nPreferredBufferSize(nPreferredBufferSize) {
  /* Disable Azure SDK logging.
      Note: This will not prevent Azure CLI, called as a subprocess by the
      Azure SDK, to log errors such as "Please run 'az login' to authenticate".
  */
  Azure::Core::Diagnostics::Logger::SetListener(
      [](Azure::Core::Diagnostics::Logger::Level, string const &) {});
}

Driver::~Driver() {}

int Driver::Exists(bool *result, const string &sUrl) const {
  ServiceRequest request;
  if (ParseUrl(&request, sUrl)) {
    return -1;
  }
  if (request.storageType == BLOB) {
    if (request.bDir) {
      *result = true; // there is no such concept as a directory when dealing
                      // with blob services
    } else {
      *result = !ListBlobs(request).empty();
    }
  } else // SHARE
  {
    if (request.bDir) {
      *result = !ListDirs(request).empty();
    } else {
      *result = !ListFiles(request).empty();
    }
  }
  return 0;
}

int Driver::GetSize(size_t *result, const string &sUrl) const {
  ServiceRequest request;
  if (ParseUrl(&request, sUrl)) {
    return -1;
  }
  if (request.bDir) {
    getLogger()->error(
        "Cannot get size of a directory: operation not supported.");
    return -1;
  }
  if (request.storageType == BLOB) {
    auto blobs = ListBlobs(request);
    if (blobs.empty()) {
      getLogger()->error("No blob matches URL {}.", sUrl);
      return -1;
    }
    *result = FragmentedFile(std::move(blobs)).GetSize();
  } else /* SHARE */ {
    auto files = ListFiles(request);
    if (files.empty()) {
      getLogger()->error("No file matches URL {}.", sUrl);
      return -1;
    }
    *result = FragmentedFile(std::move(files)).GetSize();
  }
  return 0;
}

int Driver::OpenForReading(FileStream **result, const string &sUrl) {
  try {
    ServiceRequest request;
    if (ParseUrl(&request, sUrl)) {
      return -1;
    }
    if (request.bDir) {
      getLogger()->error(
          "Cannot open a directory for reading: operation not supported.");
      return -1;
    }
    int nOpenStatus;
    FileStream fs;
    if (request.storageType == BLOB) {
      auto blobs = ListBlobs(request);
      if (blobs.empty()) {
        getLogger()->error("No blob matches URL {}.", sUrl);
        return -1;
      }
      if ((nOpenStatus = FileStream::OpenForReading(&fs, blobs))) {
        return nOpenStatus;
      }
    } else // SHARE
    {
      auto files = ListFiles(request);
      if (files.empty()) {
        getLogger()->error("No file matches URL {}.", sUrl);
        return -1;
      }
      if ((nOpenStatus = FileStream::OpenForReading(&fs, files))) {
        return nOpenStatus;
      }
    }
    *result = RegisterFileStream(std::move(fs));
    return 0;
  } catch (const exception &exc) {
    getLogger()->error("Failed to open file for reading: {}", exc.what());
    return -1;
  } catch (...) {
    getLogger()->error("Failed to open file for reading: unknown exception");
    return -1;
  }
}

int Driver::OpenForWriting(FileStream **result, const string &sUrl) {
  ServiceRequest request;
  if (ParseUrl(&request, sUrl)) {
    return -1;
  }
  if (request.bDir) {
    getLogger()->error(
        "Cannot open a directory for writing: operation not supported.");
    return -1;
  }
  FileStream fs;
  if (request.storageType == BLOB) {
    FileStream::OpenForWriting(&fs, FileStream::OutputMode::WRITE,
                               GetBlobClient(request));
  } else // SHARE
  {
    if (GetParentDir(nullptr, request)) {
      return -1;
    }
    FileStream::OpenForWriting(&fs, FileStream::OutputMode::WRITE,
                               GetFileClient(request));
  }
  *result = RegisterFileStream(std::move(fs));
  return 0;
}

int Driver::OpenForAppending(FileStream **result, const string &sUrl) {
  ServiceRequest request;
  if (ParseUrl(&request, sUrl)) {
    return -1;
  }
  if (request.bDir) {
    getLogger()->error(
        "Cannot open a directory for appending: operation not supported.");
    return -1;
  }
  FileStream fs;
  if (request.storageType == BLOB) {
    FileStream::OpenForWriting(&fs, FileStream::OutputMode::APPEND,
                               GetBlobClient(request));
  } else // SHARE
  {
    string sFilename = request.share.path.back();
    Azure::Storage::Files::Shares::ListFilesAndDirectoriesOptions opts;
    opts.Prefix = sFilename;
    bool bAlreadyExisting = false;
    Azure::Storage::Files::Shares::ShareDirectoryClient parentDir("");
    if (GetParentDir(&parentDir, request)) {
      return -1;
    }
    for (auto pagedResponse = parentDir.ListFilesAndDirectories(opts);
         pagedResponse.HasPage(); pagedResponse.MoveToNextPage()) {
      if (find_if(pagedResponse.Files.begin(), pagedResponse.Files.end(),
                  [sFilename](const auto &fileItem) {
                    return fileItem.Name == sFilename;
                  }) != pagedResponse.Files.end()) {
        bAlreadyExisting = true;
        break;
      }
    }
    auto client = GetFileClient(request);
    if (!bAlreadyExisting) {
      client.Create(0);
    }
    FileStream::OpenForWriting(&fs, FileStream::OutputMode::APPEND, client);
  }
  *result = RegisterFileStream(std::move(fs));
  return 0;
}

int Driver::Close(void *handle) {
  FileStream *fileStreamPtr;
  if (RetrieveFileStream(&fileStreamPtr, handle)) {
    return -1;
  }
  fileStreamPtr->Close();
  fileStreams.erase(fileStreamPtr->GetHandle());
  return 0;
}

int Driver::Read(size_t *nRead, void *handle, void *dest, size_t nSize,
                 size_t nCount) {
  FileStream *fileStreamPtr;
  if (RetrieveFileStream(&fileStreamPtr, handle)) {
    return -1;
  }
  if (fileStreamPtr->Read(nRead, dest, nSize, nCount)) {
    return -1;
  }
  return 0;
}

int Driver::Seek(void *handle, long long int nOffset, int nOrigin) {
  FileStream *fileStreamPtr;
  if (RetrieveFileStream(&fileStreamPtr, handle)) {
    return -1;
  }
  if (fileStreamPtr->Seek(nOffset, nOrigin)) {
    return -1;
  }
  return 0;
}

int Driver::Write(size_t *nWritten, void *handle, const void *source,
                  size_t nSize, size_t nCount) {
  FileStream *fileStreamPtr;
  if (RetrieveFileStream(&fileStreamPtr, handle)) {
    return -1;
  }
  if (fileStreamPtr->Write(nWritten, source, nSize, nCount)) {
    return -1;
  }
  return 0;
}

int Driver::Flush(void *handle) {
  FileStream *fileStreamPtr;
  if (RetrieveFileStream(&fileStreamPtr, handle)) {
    return -1;
  }
  if (fileStreamPtr->Flush()) {
    return -1;
  }
  return 0;
}

int Driver::Remove(const string &sUrl) const {
  ServiceRequest request;
  if (ParseUrl(&request, sUrl)) {
    return -1;
  }
  if (request.bDir) {
    getLogger()->error("Invalid call with a directory: use dedicated directory "
                       "removal function instead.");
    return -1;
  }
  if (request.storageType == BLOB) {
    auto blobs = ListBlobs(request);
    if (blobs.empty()) {
      getLogger()->error("No blob matches URL {}.", sUrl);
      return -1;
    }
    Azure::Storage::Blobs::DeleteBlobOptions opts;
    opts.DeleteSnapshots =
        Azure::Storage::Blobs::Models::DeleteSnapshotsOption::IncludeSnapshots;
    for (const auto &blob : blobs) {
      const string sBlobUrl = blob.GetUrl();
      if (!blob.Delete(opts).Value.Deleted) {
        getLogger()->error("Failed to delete blob {}.", sBlobUrl);
        return -1;
      }
    }
  } else /* SHARE */ {
    auto files = ListFiles(request);
    if (files.empty()) {
      getLogger()->error("No file matches URL {}.", sUrl);
      return -1;
    }
    for (const auto &file : files) {
      const string sFileUrl = file.GetUrl();
      if (!file.Delete().Value.Deleted) {
        getLogger()->error("Failed to delete file {}.", sFileUrl);
        return -1;
      }
    }
  }
  return 0;
}

int Driver::MkDir(const string &sUrl) const {
  ServiceRequest request;
  if (ParseUrl(&request, sUrl)) {
    return -1;
  }
  if (!request.bDir) {
    getLogger()->error("Cannot make a directory given a file URL.");
    return -1;
  }
  if (request.storageType == BLOB) {
    getLogger()->info("Making a directory for a blob storage does nothing.");
  } else // SHARE
  {
    string sNewDir = request.share.path.back();
    Azure::Storage::Files::Shares::ShareDirectoryClient parentDir("");
    if (GetParentDir(&parentDir, request)) {
      return -1;
    }

    Azure::Storage::Files::Shares::ListFilesAndDirectoriesOptions opts;
    opts.Prefix = sNewDir;
    for (auto pagedResponse = parentDir.ListFilesAndDirectories(opts);
         pagedResponse.HasPage(); pagedResponse.MoveToNextPage()) {
      if (find_if(pagedResponse.Directories.begin(),
                  pagedResponse.Directories.end(),
                  [sNewDir](const auto &dirItem) {
                    return dirItem.Name == sNewDir;
                  }) != pagedResponse.Directories.end()) {
        getLogger()->error("Cannot make directory: directory already exists.");
        return -1;
      }
    }

    if (!parentDir.GetSubdirectoryClient(sNewDir).Create().Value.Created) {
      getLogger()->error("Failed to make directory.");
      return -1;
    }
  }
  return 0;
}

int Driver::RmDir(const string &sUrl) const {
  ServiceRequest request;
  if (ParseUrl(&request, sUrl)) {
    return -1;
  }
  if (!request.bDir) {
    getLogger()->error("Cannot remove a directory given a file URL.");
    return -1;
  }
  if (request.storageType == BLOB) {
    getLogger()->info("Removing a directory with a blob storage does nothing.");
  } else // SHARE
  {
    auto dirs = ListDirs(request);
    if (dirs.empty()) {
      getLogger()->error("No file matches URL {}.", sUrl);
      return -1;
    }
    for (const auto &dir : dirs) {
      const string sDirUrl = dir.GetUrl();
      if (!dir.Delete().Value.Deleted) {
        getLogger()->error("Failed to delete directory {}.", sDirUrl);
        return -1;
      }
    }
  }
  return 0;
}

int Driver::GetFreeDiskSpace(size_t *result) const {
  *result = 5ULL * 1024ULL * 1024ULL * 1024ULL * 1024ULL;
  return 0;
}

int Driver::CopyTo(const string &sUrl, const string &destUrl) {
  try {
    ServiceRequest request;
    if (ParseUrl(&request, sUrl)) {
      return -1;
    }
    if (request.bDir) {
      getLogger()->error(
          "Cannot copy a directory to a local file: operation not supported.");
      return -1;
    }
    FileStream *readerPtr;
    if (OpenForReading(&readerPtr, sUrl)) {
      return -1;
    }
    std::unique_ptr<char[]> buffer = std::make_unique<char[]>(nPreferredBufferSize);
    ofstream ofs(destUrl, ios::binary);
    size_t nRead;

    for (;;) {
      getLogger()->trace("Copying at most {} bytes from remote to local file...",
                         nPreferredBufferSize);
      switch (readerPtr->Read(&nRead, buffer.get(), 1, nPreferredBufferSize)) {
      case 0:
        ofs.write(buffer.get(), (streamsize)nRead);
        continue;
      case -1:
        return -1;
      case -2: // Read at EOF
        break;
      }
      break;
    }

    if (Close(readerPtr->GetHandle())) {
      return -1;
    }
    return 0;
  } catch (const exception &exc) {
    getLogger()->error("Copy operation failed: {}", exc.what());
    return -1;
  } catch (...) {
    getLogger()->error("Copy operation failed: unknown exception");
    return -1;
  }
}

int Driver::CopyFrom(const string &sUrl, const string &sourceUrl) {
  ServiceRequest request;
  if (ParseUrl(&request, sUrl)) {
    return -1;
  }
  if (request.bDir) {
    getLogger()->error(
        "Cannot copy from a local file to a directory: operation not "
        "supported.");
    return -1;
  }
  FileStream *writerPtr;
  if (OpenForWriting(&writerPtr, sUrl)) {
    return -1;
  }
  char *buffer = new char[nPreferredBufferSize];
  size_t nRead;
  ifstream ifs(sourceUrl, ios::binary);

  for (;;) {
    getLogger()->trace("Copying at most {} bytes from local file to remote...",
                       nPreferredBufferSize);
    ifs.read(buffer, nPreferredBufferSize);
    nRead = (size_t)ifs.gcount();
    if (nRead == 0) {
      break;
    }
    int nWriteStatus;
    size_t nWritten;
    if ((nWriteStatus = writerPtr->Write(&nWritten, buffer, 1, nRead))) {
      return nWriteStatus;
    }
  }

  delete[] buffer;
  if (Close(writerPtr->GetHandle())) {
    return -1;
  }
  return 0;
}

int Driver::Concatenate(const vector<string> &inputUrls,
                        const string &sDestUrl) {
  size_t nInputUrls = inputUrls.size();
  if (nInputUrls < 2) {
    getLogger()->info("Number of input URLs is {}; do not concatenate.",
                      nInputUrls);
    return 0;
  }

  vector<ServiceRequest> inputs(nInputUrls);
  for (size_t i = 0ULL; i < nInputUrls; i++) {
    if (ParseUrl(&inputs[i], inputUrls[i])) {
      return -1;
    }
  }
  ServiceRequest output;
  if (ParseUrl(&output, sDestUrl)) {
    return -1;
  }

  for (const auto &input : inputs) {
    if (input.storageType != output.storageType) {
      getLogger()->error(
          "Input storage type (blob/file) does not match output storage type.");
      return -1;
    }
    if (input.bDir) {
      getLogger()->error(
          "Cannot concatenate directories: operation not supported.");
      return -1;
    }
  }
  if (output.bDir) {
    getLogger()->error(
        "Cannot concatenate to a directory: operation not supported.");
    return -1;
  }

  try {
    if (output.storageType == BLOB) {
      Azure::Storage::Blobs::BlockBlobClient destBlob =
          GetBlobClient(output).AsBlockBlobClient();
      vector<string> destBlockIds;

      for (const ServiceRequest &input : inputs) {
        Auth sourceAuth;
        if (BuildAuth(&sourceAuth, input)) {
          return -1;
        }
        Azure::Storage::Blobs::StageBlockFromUriOptions opts;
        if (sourceAuth.HasHeader()) {
          opts.SourceAuthorization = sourceAuth.sAuthHeader;
        }

        ostringstream oss;
        oss << setfill('0') << setw(64) << destBlockIds.size();
        string sBlockIdInBase10 = oss.str();
        vector<uint8_t> blockIdInBase10(sBlockIdInBase10.begin(),
                                        sBlockIdInBase10.end());
        string sBlockIdInBase64 =
            Azure::Core::Convert::Base64Encode(blockIdInBase10);
        destBlockIds.push_back(sBlockIdInBase64);

        destBlob.StageBlockFromUri(sBlockIdInBase64, sourceAuth.sUriAuth, opts);
      }
      destBlob.CommitBlockList(destBlockIds);
    } else /* SHARE */ {
      Azure::Core::Http::HttpRange range;
      getLogger()->trace("Concatenating / Output: {}",
                         output.azureUrl.GetAbsoluteUrl());
      Azure::Storage::Files::Shares::ShareFileClient destFile =
          GetFileClient(output);

      unordered_map<const ServiceRequest *, int64_t> sourceSizes;
      int64_t nTotalSize = 0LL;
      for (const ServiceRequest &input : inputs) {
        getLogger()->trace("Concatenating / Input: {}",
                           input.azureUrl.GetAbsoluteUrl());
        int64_t nSourceSize =
            GetFileClient(input).GetProperties().Value.FileSize;
        getLogger()->trace("Concatenating / Size of input: {}", nSourceSize);
        sourceSizes[&input] = nSourceSize;
        nTotalSize += nSourceSize;
      }
      destFile.Create(nTotalSize);
      getLogger()->trace("Concatenating / Created destination file of size: {}",
                         nTotalSize);
      int64_t nGlobalOffset = 0LL;
      for (const ServiceRequest &input : inputs) {
        int64_t nSourceSize = sourceSizes[&input];
        Auth sourceAuth;
        if (BuildAuth(&sourceAuth, input)) {
          return -1;
        }
        Azure::Storage::Files::Shares::UploadFileRangeFromUriOptions opts;
        if (sourceAuth.HasHeader()) {
          opts.SourceAuthorization = sourceAuth.sAuthHeader;
        }
        // See size limitation of source range: header x-ms-source-range at
        // https://learn.microsoft.com/en-us/rest/api/storageservices/put-range-from-url
        // .
        constexpr int64_t MAX_SOURCE_SIZE = 4LL * 1024LL * 1024LL;
        for (int64_t nOffsetInSource = 0LL; nOffsetInSource < nSourceSize;
             nOffsetInSource += MAX_SOURCE_SIZE) {
          int64_t nToUpload =
              min(nSourceSize - nOffsetInSource, MAX_SOURCE_SIZE);
          range = Azure::Core::Http::HttpRange{nOffsetInSource, nToUpload};
          destFile.UploadRangeFromUri(nGlobalOffset + nOffsetInSource,
                                      sourceAuth.sUriAuth, range, opts);
        }
        nGlobalOffset += nSourceSize;
      }
    }
  } catch (const Azure::Core::RequestFailedException &exc) {
    getLogger()->error(
        "Failed to upload range from URI. Details of Azure error:");
    getLogger()->error("  Exception message: {}", exc.what());
    getLogger()->error("  HTTP response headers:");
    for (const auto &header : exc.RawResponse->GetHeaders()) {
      getLogger()->error("    Header name: '{}'   Header value: '{}'",
                         header.first, header.second);
    }
    return -1;
  }

  for (const string &sInputUrl : inputUrls) {
    if (Remove(sInputUrl)) {
      return -1;
    }
  }

  return 0;
}

bool Driver::IsEmulatedStorage() const {
  string sEmulatedStorageEnvVarVal = env::GetEnvVar("AZURE_EMULATED_STORAGE");
  return !sEmulatedStorageEnvVarVal.empty() &&
         sEmulatedStorageEnvVarVal != "false";
}

int Driver::ParseUrl(ServiceRequest *result, const std::string &sUrl) const {
  // Basic URL parsing
  Azure::Core::Url url;
  try {
    url = Azure::Core::Url(sUrl);
  } catch (const exception &) {
    getLogger()->error(
        "Caught an exception while performing basic URL parsing: URL "
        "{} is invalid.",
        sUrl);
    return -1;
  }
  const string &sPath = url.GetPath();

  // Determine some properties about the requested resource
  bool bDir = str::EndsWith(sPath, "/");
  bool bIsEmulatedStorage = IsEmulatedStorage();
  bool bAzureBlobUrl = str::EndsWith(url.GetHost(), ".blob.core.windows.net");
  bool bAzureFileUrl =
      !bAzureBlobUrl && str::EndsWith(url.GetHost(), ".file.core.windows.net");
  if (!bIsEmulatedStorage && !bAzureBlobUrl && !bAzureFileUrl) {
    getLogger()->error("URL {} contains invalid domain.", sUrl);
    return -1;
  }

  // Determine storage type
  StorageType storageType;
  if (bIsEmulatedStorage) {
    storageType = BLOB; // The emulator supports only blob storages
  } else if (bAzureBlobUrl) {
    storageType = BLOB;
  } else {
    storageType = SHARE;
  }

  // Connection string parsing
  string sConnectionString = env::GetEnvVar("AZURE_STORAGE_CONNECTION_STRING");
  bool bConnectionStringDefined = !sConnectionString.empty();
  connstr::ConnectionString connectionString;
  if (bConnectionStringDefined) {
    if (connstr::ConnectionString::ParseConnectionString(
            &connectionString, sConnectionString, bIsEmulatedStorage)) {
      return -1;
    }
    if (connectionString.CheckAgainstUrl(url, storageType)) {
      return -1;
    }
  }

  // Credentials
  shared_ptr<Azure::Storage::StorageSharedKeyCredential>
      connectionStringCredential;
  shared_ptr<Azure::Core::Credentials::TokenCredential>
      noConnectionStringCredential;
  if (bConnectionStringDefined) {
    connectionStringCredential =
        make_shared<Azure::Storage::StorageSharedKeyCredential>(
            connectionString.sAccountName, connectionString.sAccountKey);
  } else {
    noConnectionStringCredential =
        make_shared<Azure::Identity::ChainedTokenCredential>(
            Azure::Identity::ChainedTokenCredential::Sources{
                std::make_shared<
                    Azure::Identity::
                        EnvironmentCredential>(), // for Client ID + Client
                                                  // Secret or Certificate
                                                  // environment variables
                std::make_shared<Azure::Identity::WorkloadIdentityCredential>(),
                std::make_shared<Azure::Identity::ManagedIdentityCredential>(),
                std::make_shared<Azure::Identity::AzureCliCredential>()});
  }

  // Split stored object path
  if (bIsEmulatedStorage) // Emulated BLOB storage
  {
    if (!bConnectionStringDefined) {
      getLogger()->error("Undefined of empty environment variable: "
                         "AZURE_STORAGE_CONNECTION_STRING.");
      return -1;
    }
    smatch match;
    if (!regex_match(
            sPath, match,
            regex("([^/]+)/([^/]+)/(.+)"))) //  accountname/container/object  or
                                            //  accountname/container/object/
    {
      getLogger()->error("Invalid emulated storage object path: {}.", sPath);
      return -1;
    }
    *result =
        ServiceRequest(url, bIsEmulatedStorage, BLOB, bDir,
                       BlobInfo{match[1].str(), match[2].str(), match[3].str()},
                       connectionStringCredential);
  } else if (bAzureBlobUrl) { // Real Azure cloud BLOB storage
    smatch match;
    if (!regex_match(
            sPath, match,
            regex("([^/]+)/(.+)"))) //  container/object  or  container/object/
    {
      getLogger()->error("Invalid cloud blob path: {}.", sPath);
      return -1;
    }
    if (bConnectionStringDefined) {
      *result =
          ServiceRequest(url, bIsEmulatedStorage, BLOB, bDir,
                         BlobInfo{string(), match[1].str(), match[2].str()},
                         connectionStringCredential);
    } else {
      *result =
          ServiceRequest(url, bIsEmulatedStorage, BLOB, bDir,
                         BlobInfo{string(), match[1].str(), match[2].str()},
                         noConnectionStringCredential);
    }
  } else { // Real Azure cloud SHARE storage
    smatch match;
    if (!regex_match(
            sPath, match,
            regex("([^/]+)((?:/[^/]+)+/?)"))) //  share/path/to/a/file  or
                                              //  share/path/to/a/dir/
    {
      getLogger()->error("Invalid cloud file path: {}.", sPath);
      return -1;
    }
    vector<string> fileOrDirPath = str::Split(match[2].str(), '/', -1, true);
    if (bConnectionStringDefined) {
      *result = ServiceRequest(url, bIsEmulatedStorage, SHARE, bDir,
                               ShareInfo{match[1].str(), fileOrDirPath},
                               connectionStringCredential);
    } else {
      *result = ServiceRequest(url, bIsEmulatedStorage, SHARE, bDir,
                               ShareInfo{match[1].str(), fileOrDirPath},
                               noConnectionStringCredential);
    }
  }
  return 0;
}

string Driver::GetServiceUrl(const ServiceRequest &request) const {
  ostringstream oss;
  if (request.bEmulated) { // Emulated BLOB storage
    oss << request.azureUrl.GetScheme() << "://" << request.azureUrl.GetHost()
        << ":" << request.azureUrl.GetPort() << "/"
        << request.blob.sAccountName;
  } else if (request.storageType == BLOB) { // Real Azure cloud BLOB storage
    oss << request.azureUrl.GetScheme() << "://" << request.azureUrl.GetHost()
        << ":" << request.azureUrl.GetPort();
  } else { // Real Azure cloud FILE storage
    oss << request.azureUrl.GetScheme() << "://" << request.azureUrl.GetHost()
        << ":" << request.azureUrl.GetPort();
  }
  std::string result = oss.str();
  getLogger()->debug("Service URL is: {}.", result);
  return result;
}

string Driver::GetBlobContainerUrl(const ServiceRequest &request) const {
  ostringstream oss;
  oss << GetServiceUrl(request) << "/" << request.blob.sContainer;
  std::string result = oss.str();
  getLogger()->debug("Blob container URL is: {}.", result);
  return result;
}

Azure::Storage::Blobs::BlobServiceClient
Driver::GetBlobServiceClient(const ServiceRequest &request) const {
  if (request.bUsingConnectionString) {
    return Azure::Storage::Blobs::BlobServiceClient(GetServiceUrl(request), request.connectionStringCredential, MakeBlobClientOptions());
  } else {
    return Azure::Storage::Blobs::BlobServiceClient(GetServiceUrl(request), request.noConnectionStringCredential, MakeBlobClientOptions());
  }
}

Azure::Storage::Blobs::BlobContainerClient
Driver::GetBlobContainerClient(const ServiceRequest &request) const {
  if (request.bUsingConnectionString) {
    return Azure::Storage::Blobs::BlobContainerClient(
        GetBlobContainerUrl(request), request.connectionStringCredential, MakeBlobClientOptions());
  } else {
    return Azure::Storage::Blobs::BlobContainerClient(
        GetBlobContainerUrl(request), request.noConnectionStringCredential, MakeBlobClientOptions());
  }
}

Azure::Storage::Blobs::BlobClient
Driver::GetBlobClient(const ServiceRequest &request) const {
  if (request.bUsingConnectionString) {
    return Azure::Storage::Blobs::BlobClient(
        request.azureUrl.GetAbsoluteUrl(), request.connectionStringCredential, MakeBlobClientOptions());
  } else {
    return Azure::Storage::Blobs::BlobClient(
        request.azureUrl.GetAbsoluteUrl(),
        request.noConnectionStringCredential, MakeBlobClientOptions());
  }
}

vector<Azure::Storage::Blobs::BlobClient>
Driver::ListBlobs(const ServiceRequest &request) const {
  return ResolveBlobsSearchString(GetBlobContainerClient(request),
                                  request.blob.sBlob);
}

string Driver::GetFileShareUrl(const ServiceRequest &request) const {
  ostringstream oss;
  oss << GetServiceUrl(request) << "/" << request.share.sShare;
  string result = oss.str();
  getLogger()->debug("File share URL is: {}.", result);
  return result;
}

Azure::Storage::Files::Shares::ShareServiceClient
Driver::GetFileShareServiceClient(const ServiceRequest &request) const {
  if (request.bUsingConnectionString) {
    return Azure::Storage::Files::Shares::ShareServiceClient(
        GetServiceUrl(request), request.connectionStringCredential, MakeShareClientOptions());
  } else {
    return Azure::Storage::Files::Shares::ShareServiceClient(
        GetServiceUrl(request), request.noConnectionStringCredential, MakeShareClientOptions());
  }
}

Azure::Storage::Files::Shares::ShareClient
Driver::GetShareClient(const ServiceRequest &request) const {
  if (request.bUsingConnectionString) {
    return Azure::Storage::Files::Shares::ShareClient(
        GetFileShareUrl(request), request.connectionStringCredential, MakeShareClientOptions());
  } else {
    return Azure::Storage::Files::Shares::ShareClient(
        GetFileShareUrl(request), request.noConnectionStringCredential, MakeShareClientOptions());
  }
}

Azure::Storage::Files::Shares::ShareDirectoryClient
Driver::GetDirClient(const ServiceRequest &request) const {
  return GetShareClient(request).GetRootDirectoryClient();
}

Azure::Storage::Files::Shares::ShareFileClient
Driver::GetFileClient(const ServiceRequest &request) const {
  if (request.bUsingConnectionString) {
    return Azure::Storage::Files::Shares::ShareFileClient(
        request.azureUrl.GetAbsoluteUrl(), request.connectionStringCredential, MakeShareClientOptions());
  } else {
    return Azure::Storage::Files::Shares::ShareFileClient(
        request.azureUrl.GetAbsoluteUrl(), request.noConnectionStringCredential, MakeShareClientOptions());
  }
}

vector<Azure::Storage::Files::Shares::ShareDirectoryClient>
Driver::ListDirs(const ServiceRequest &request) const {
  return ResolveDirsPathRecursively(
      GetDirClient(request),
      queue<string, deque<string>>(
          deque<string>(request.share.path.begin(), request.share.path.end())));
}

vector<Azure::Storage::Files::Shares::ShareFileClient>
Driver::ListFiles(const ServiceRequest &request) const {
  return ResolveFilesPathRecursively(
      GetDirClient(request),
      queue<string, deque<string>>(
          deque<string>(request.share.path.begin(), request.share.path.end())));
}

int Driver::GetParentDir(
    Azure::Storage::Files::Shares::ShareDirectoryClient *result,
    const ServiceRequest &request) const {
  Azure::Storage::Files::Shares::ShareDirectoryClient dirClient =
      GetDirClient(request);
  vector<string> path = request.share.path;
  path.pop_back();

  for (string sPathFragment : path) {
    Azure::Storage::Files::Shares::ListFilesAndDirectoriesOptions opts;
    opts.Prefix = sPathFragment;

    bool bAlreadyExisting = false;
    for (auto pagedResponse = dirClient.ListFilesAndDirectories(opts);
         pagedResponse.HasPage(); pagedResponse.MoveToNextPage()) {
      if (find_if(pagedResponse.Directories.begin(),
                  pagedResponse.Directories.end(),
                  [sPathFragment](const auto &dirItem) {
                    return dirItem.Name == sPathFragment;
                  }) != pagedResponse.Directories.end()) {
        bAlreadyExisting = true;
        break;
      }
    }

    if (!bAlreadyExisting) {
      getLogger()->error("Ancestor directory {}/{} does not exist.",
                         dirClient.GetUrl(), sPathFragment,
                         request.azureUrl.GetAbsoluteUrl());
      return -1;
    }

    dirClient = dirClient.GetSubdirectoryClient(sPathFragment);
  }

  if (result)
    *result = dirClient;
  return 0;
}

FileStream *Driver::RegisterFileStream(FileStream &&fileStream) {
  void *handle = fileStream.GetHandle();
  fileStreams[handle] = make_unique<FileStream>(std::move(fileStream));
  return fileStreams.at(handle).get();
}

int Driver::RetrieveFileStream(FileStream **result, void *handle) const {
  auto it = fileStreams.find(handle);
  if (it == fileStreams.end()) {
    getLogger()->error("File stream not found.");
    return -1;
  }
  *result = it->second.get();
  return 0;
}

} // namespace az
