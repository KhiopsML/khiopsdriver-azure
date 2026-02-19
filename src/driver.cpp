#include "driver.hpp"
#include "blobpathresolve.hpp"
#include "sharepathresolve.hpp"
#include "storagetype.hpp"
#include "util.hpp"
#include <algorithm>
#include <azure/core.hpp>
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
#include <sstream>
#include <stdexcept>

using namespace std;
using namespace az::util;

namespace az {
ServiceRequest::ServiceRequest(
    const Azure::Core::Url azureUrl, bool bEmulated, StorageType storageType,
    bool bDir, const BlobInfo &blob,
    shared_ptr<Azure::Storage::StorageSharedKeyCredential>
        connectionStringCredential)
    : azureUrl(azureUrl), bEmulated(bEmulated), storageType(storageType),
      bUsingConnectionString(true), bDir(bDir),
      blob{blob.sAccountName, blob.sContainer, blob.sBlob},
      connectionStringCredential(std::move(connectionStringCredential)) {
  Info();
}

ServiceRequest::ServiceRequest(
    const Azure::Core::Url azureUrl, bool bEmulated, StorageType storageType,
    bool bDir, const BlobInfo &blob,
    shared_ptr<Azure::Core::Credentials::TokenCredential>
        noConnectionStringCredential)
    : azureUrl(azureUrl), bEmulated(bEmulated), storageType(storageType),
      bUsingConnectionString(false), bDir(bDir),
      blob{blob.sAccountName, blob.sContainer, blob.sBlob},
      noConnectionStringCredential(std::move(noConnectionStringCredential)) {
  Info();
}

ServiceRequest::ServiceRequest(
    const Azure::Core::Url azureUrl, bool bEmulated, StorageType storageType,
    bool bDir, const ShareInfo &share,
    shared_ptr<Azure::Storage::StorageSharedKeyCredential>
        connectionStringCredential)
    : azureUrl(azureUrl), bEmulated(bEmulated), storageType(storageType),
      bUsingConnectionString(true), bDir(bDir), share{share.sShare, share.path},
      connectionStringCredential(std::move(connectionStringCredential)) {
  Info();
}

ServiceRequest::ServiceRequest(
    const Azure::Core::Url azureUrl, bool bEmulated, StorageType storageType,
    bool bDir, const ShareInfo &share,
    shared_ptr<Azure::Core::Credentials::TokenCredential>
        noConnectionStringCredential)
    : azureUrl(azureUrl), bEmulated(bEmulated), storageType(storageType),
      bUsingConnectionString(false), bDir(bDir),
      share{share.sShare, share.path},
      noConnectionStringCredential(std::move(noConnectionStringCredential)) {
  Info();
}

ServiceRequest::ServiceRequest(const ServiceRequest &other)
    : azureUrl(other.azureUrl), bEmulated(other.bEmulated),
      storageType(other.storageType),
      bUsingConnectionString(other.bUsingConnectionString), bDir(other.bDir),
      blob(other.blob), share(other.share),
      connectionStringCredential(other.connectionStringCredential),
      noConnectionStringCredential(other.noConnectionStringCredential) {
  Info();
}

ServiceRequest::ServiceRequest() {}

ServiceRequest &ServiceRequest::operator=(ServiceRequest &&other) {
  azureUrl = std::move(other.azureUrl);
  bEmulated = std::move(other.bEmulated);
  storageType = std::move(other.storageType);
  bUsingConnectionString = std::move(other.bUsingConnectionString);
  bDir = std::move(other.bDir);
  blob = std::move(other.blob);
  share = std::move(other.share);
  connectionStringCredential = std::move(other.connectionStringCredential);
  noConnectionStringCredential = std::move(other.noConnectionStringCredential);
  return *this;
}

void ServiceRequest::Info() {
  spdlog::debug("Created service request:");
  spdlog::debug("  URL: {}", azureUrl.GetAbsoluteUrl());
  spdlog::debug("  emulated: {}", bEmulated ? "yes" : "no");
  switch (storageType) {
  case BLOB:
    spdlog::debug("  storage type: blob");
    break;
  case SHARE:
    spdlog::debug("  storage type: file");
    break;
  default:
    spdlog::debug("  storage type: <invalid: {}>",
                  static_cast<int>(storageType));
    break;
  }
  spdlog::debug("  using connection string: {}",
                bUsingConnectionString ? "yes" : "no");
  spdlog::debug("  directory: {}", bDir ? "yes" : "no");
  if (storageType == BLOB) {
    spdlog::debug("  blob info:");
    spdlog::debug("    account name: {}", blob.sAccountName);
    spdlog::debug("    container: {}", blob.sContainer);
    spdlog::debug("    blob: {}", blob.sBlob);
  } else {
    spdlog::debug("  file info:");
    spdlog::debug("    share: {}", share.sShare);
    spdlog::debug("    path:");
    for (size_t i = 0ULL; i < share.path.size(); i++) {
      spdlog::debug("      element #{}: {}", i + 1, share.path.at(i));
    }
  }
}

Driver::Driver()
    : bIsConnected(false),
      nPreferredBufferSize(nDefaultPreferredBufferSize) /* Default value */ {
  InitializeLogging();
  string envvarval = env::GetEnvVar("AZURE_PREFERRED_BUFFER_SIZE");
  if (!envvarval.empty()) {
    try {
      nPreferredBufferSize = stoull(envvarval);
    } catch (const invalid_argument &exc) {
      spdlog::debug(
          "Value {} of environment variable AZURE_PREFERRED_BUFFER_SIZE is not "
          "a valid number. Falling back to default {}...",
          envvarval, nDefaultPreferredBufferSize);
    } catch (const out_of_range &exc) {
      spdlog::debug(
          "Value {} of environment variable AZURE_PREFERRED_BUFFER_SIZE is out "
          "of range. Falling back to default {}...",
          envvarval, nDefaultPreferredBufferSize);
    }
  }
}

Driver::~Driver() { fileStreams.clear(); }

void Driver::InitializeLogging() {
  logstring = "";
  logstringstream = ostringstream("");
  stringstreamsink =
      make_shared<spdlog::sinks::ostream_sink_st>(logstringstream);
  stringstreamsink->set_level(spdlog::level::err);
  defaultloggersinks.push_back(stringstreamsink);

  stderrsink = make_shared<spdlog::sinks::stderr_sink_st>();
  stderrsink->set_level(spdlog::level::from_str(
      env::GetEnvVarOrDefault("AZURE_DRIVER_LOGLEVEL", "off")));
  defaultloggersinks.push_back(stderrsink);

  string sLogFileEnvVal = env::GetEnvVar("AZURE_DRIVER_LOGFILE");
  if (!sLogFileEnvVal.empty()) {
    filesink = make_shared<spdlog::sinks::basic_file_sink_st>(sLogFileEnvVal);
    filesink->set_level(spdlog::level::trace);
    defaultloggersinks.push_back(filesink);
  }

  defaultloggersinks = {stringstreamsink, stderrsink};

  defaultlogger = make_shared<spdlog::logger>(
      "default", defaultloggersinks.begin(), defaultloggersinks.end());
  defaultlogger->set_level(
      spdlog::level::trace); // Let the sinks choose the log level.
  spdlog::set_default_logger(defaultlogger);

  /* Disable Azure SDK logging.
     Note: This will not prevent Azure CLI, called as a subprocess by the
     Azure SDK, to log errors such as "Please run 'az login' to authenticate".
   */
  Azure::Core::Diagnostics::Logger::SetListener(
      [](Azure::Core::Diagnostics::Logger::Level, std::string const &) {});
}

const string &Driver::GetName() const {
  static string sName = "Azure driver";
  return sName;
}

const string &Driver::GetVersion() const {
  static string sVersion = DRIVER_VERSION;
  return sVersion;
}

const string &Driver::GetScheme() const {
  static string sScheme = "https";
  return sScheme;
}

bool Driver::IsReadOnly() const { return false; }

size_t Driver::GetPreferredBufferSize() const { return nPreferredBufferSize; }

void Driver::Connect() { bIsConnected = true; }

int Driver::Disconnect() {
  if (!IsConnected()) {
    spdlog::error("Cannot disconnect when already disconnected.");
    return -1;
  }
  bIsConnected = false;
  return 0;
}

bool Driver::IsConnected() const { return bIsConnected; }

int Driver::Exists(bool *result, const string &sUrl) const {
  if (!IsConnected()) {
    spdlog::error("Cannot check for object existence when disconnected.");
    return -1;
  }
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
  if (!IsConnected()) {
    spdlog::error("Cannot get object size when disconnected.");
    return -1;
  }
  ServiceRequest request;
  if (ParseUrl(&request, sUrl)) {
    return -1;
  }
  if (request.bDir) {
    spdlog::error("Cannot get size of a directory: operation not supported.");
    return -1;
  }
  if (request.storageType == BLOB) {
    auto blobs = ListBlobs(request);
    if (blobs.empty()) {
      spdlog::error("No blob matches URL {}.", sUrl);
      return -1;
    }
    *result = FragmentedFile(std::move(blobs)).GetSize();
  } else /* SHARE */ {
    auto files = ListFiles(request);
    if (files.empty()) {
      spdlog::error("No file matches URL {}.", sUrl);
      return -1;
    }
    *result = FragmentedFile(std::move(files)).GetSize();
  }
  return 0;
}

int Driver::OpenForReading(FileStream **result, const string &sUrl) {
  if (!IsConnected()) {
    spdlog::error("Cannot open object for reading when disconnected.");
    return -1;
  }
  ServiceRequest request;
  if (ParseUrl(&request, sUrl)) {
    return -1;
  }
  if (request.bDir) {
    spdlog::error(
        "Cannot open a directory for reading: operation not supported.");
    return -1;
  }
  int nOpenStatus;
  FileStream fs;
  if (request.storageType == BLOB) {
    auto blobs = ListBlobs(request);
    if (blobs.empty()) {
      spdlog::error("No blob matches URL {}.", sUrl);
      return -1;
    }
    if ((nOpenStatus = FileStream::OpenForReading(&fs, blobs))) {
      return nOpenStatus;
    }
  } else // SHARE
  {
    auto files = ListFiles(request);
    if (files.empty()) {
      spdlog::error("No file matches URL {}.", sUrl);
      return -1;
    }
    if ((nOpenStatus = FileStream::OpenForReading(&fs, files))) {
      return nOpenStatus;
    }
  }
  *result = RegisterFileStream(std::move(fs));
  return 0;
}

int Driver::OpenForWriting(FileStream **result, const string &sUrl) {
  if (!IsConnected()) {
    spdlog::error("Cannot open object for writing when disconnected.");
    return -1;
  }
  ServiceRequest request;
  if (ParseUrl(&request, sUrl)) {
    return -1;
  }
  if (request.bDir) {
    spdlog::error(
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
  if (!IsConnected()) {
    spdlog::error("Cannot open object for appending when disconnected.");
    return -1;
  }
  ServiceRequest request;
  if (ParseUrl(&request, sUrl)) {
    return -1;
  }
  if (request.bDir) {
    spdlog::error(
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

void Driver::GetLastError(string **result) {
  logstring = logstringstream.str();
  logstringstream.str("");
  *result = &logstring;
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
  if (!IsConnected()) {
    spdlog::error("Cannot remove file when disconnected.");
    return -1;
  }
  ServiceRequest request;
  if (ParseUrl(&request, sUrl)) {
    return -1;
  }
  if (request.bDir) {
    spdlog::error("Invalid call with a directory: use dedicated directory "
                  "removal function instead.");
    return -1;
  }
  if (request.storageType == BLOB) {
    auto blobs = ListBlobs(request);
    if (blobs.empty()) {
      spdlog::error("No blob matches URL {}.", sUrl);
      return -1;
    }
    Azure::Storage::Blobs::DeleteBlobOptions opts;
    opts.DeleteSnapshots =
        Azure::Storage::Blobs::Models::DeleteSnapshotsOption::IncludeSnapshots;
    for (const auto &blob : blobs) {
      const string sBlobUrl = blob.GetUrl();
      if (!blob.Delete(opts).Value.Deleted) {
        spdlog::error("Failed to delete blob {}.", sBlobUrl);
        return -1;
      }
    }
  } else /* SHARE */ {
    auto files = ListFiles(request);
    if (files.empty()) {
      spdlog::error("No file matches URL {}.", sUrl);
      return -1;
    }
    for (const auto &file : files) {
      const string sFileUrl = file.GetUrl();
      if (!file.Delete().Value.Deleted) {
        spdlog::error("Failed to delete file {}.", sFileUrl);
        return -1;
      }
    }
  }
  return 0;
}

int Driver::MkDir(const string &sUrl) const {
  if (!IsConnected()) {
    spdlog::error("Cannot make a directory when disconnected.");
    return -1;
  }
  ServiceRequest request;
  if (ParseUrl(&request, sUrl)) {
    return -1;
  }
  if (!request.bDir) {
    spdlog::error("Cannot make a directory given a file URL.");
    return -1;
  }
  if (request.storageType == BLOB) {
    spdlog::info("Making a directory for a blob storage does nothing.");
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
        spdlog::error("Cannot make directory: directory already exists.");
        return -1;
      }
    }

    if (!parentDir.GetSubdirectoryClient(sNewDir).Create().Value.Created) {
      spdlog::error("Failed to make directory.");
      return -1;
    }
  }
  return 0;
}

int Driver::RmDir(const string &sUrl) const {
  if (!IsConnected()) {
    spdlog::error("Cannot remove directory when disconnected.");
    return -1;
  }
  ServiceRequest request;
  if (ParseUrl(&request, sUrl)) {
    return -1;
  }
  if (!request.bDir) {
    spdlog::error("Cannot remove a directory given a file URL.");
    return -1;
  }
  if (request.storageType == BLOB) {
    spdlog::info("Removing a directory with a blob storage does nothing.");
  } else // SHARE
  {
    auto dirs = ListDirs(request);
    if (dirs.empty()) {
      spdlog::error("No file matches URL {}.", sUrl);
      return -1;
    }
    for (const auto &dir : dirs) {
      const string sDirUrl = dir.GetUrl();
      if (!dir.Delete().Value.Deleted) {
        spdlog::error("Failed to delete directory {}.", sDirUrl);
        return -1;
      }
    }
  }
  return 0;
}

int Driver::GetFreeDiskSpace(size_t *result) const {
  if (!IsConnected()) {
    spdlog::error("Cannot get free disk space when disconnected.");
    return -1;
  }
  *result = 5ULL * 1024ULL * 1024ULL * 1024ULL * 1024ULL;
  return 0;
}

int Driver::CopyTo(const string &sUrl, const string &destUrl) {
  if (!IsConnected()) {
    spdlog::error("Cannot copy to a local file when disconnected.");
    return -1;
  }
  ServiceRequest request;
  if (ParseUrl(&request, sUrl)) {
    return -1;
  }
  if (request.bDir) {
    spdlog::error(
        "Cannot copy a directory to a local file: operation not supported.");
    return -1;
  }
  FileStream *readerPtr;
  if (OpenForReading(&readerPtr, sUrl)) {
    return -1;
  }
  char *buffer = new char[GetPreferredBufferSize()];
  ofstream ofs(destUrl, ios::binary);
  size_t nRead;

  for (;;) {
    spdlog::trace("Copying at most {} bytes from remote to local file...",
                  GetPreferredBufferSize());
    switch (readerPtr->Read(&nRead, buffer, 1, GetPreferredBufferSize())) {
    case 0:
      ofs.write(buffer, (streamsize)nRead);
      continue;
    case -1:
      return -1;
    case -2: // Read at EOF
      break;
    }
    break;
  }

  delete[] buffer;
  if (Close(readerPtr->GetHandle())) {
    return -1;
  }
  return 0;
}

int Driver::CopyFrom(const string &sUrl, const string &sourceUrl) {
  if (!IsConnected()) {
    spdlog::error("Cannot copy from a local file when disconnected.");
    return -1;
  }
  ServiceRequest request;
  if (ParseUrl(&request, sUrl)) {
    return -1;
  }
  if (request.bDir) {
    spdlog::error("Cannot copy from a local file to a directory: operation not "
                  "supported.");
    return -1;
  }
  FileStream *writerPtr;
  if (OpenForWriting(&writerPtr, sUrl)) {
    return -1;
  }
  char *buffer = new char[GetPreferredBufferSize()];
  size_t nRead;
  ifstream ifs(sourceUrl, ios::binary);

  for (;;) {
    spdlog::trace("Copying at most {} bytes from local file to remote...",
                  GetPreferredBufferSize());
    ifs.read(buffer, GetPreferredBufferSize());
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
  if (!IsConnected()) {
    spdlog::error("Cannot concatenate objects when disconnected.");
    return -1;
  }
  size_t nInputUrls = inputUrls.size();
  if (nInputUrls < 2) {
    spdlog::info("Number of input URLs is {}; do not concatenate.", nInputUrls);
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
      spdlog::error(
          "Input storage type (blob/file) does not match output storage type.");
      return -1;
    }
    if (input.bDir) {
      spdlog::error("Cannot concatenate directories: operation not supported.");
      return -1;
    }
  }
  if (output.bDir) {
    spdlog::error(
        "Cannot concatenate to a directory: operation not supported.");
    return -1;
  }

  vector<FragmentedFile> fragmentedFiles;
  if (output.storageType == BLOB) {
    transform(
        inputs.begin(), inputs.end(), back_inserter(fragmentedFiles),
        [this](const auto &input) { return FragmentedFile(ListBlobs(input)); });
  } else // SHARE
  {
    transform(
        inputs.begin(), inputs.end(), back_inserter(fragmentedFiles),
        [this](const auto &input) { return FragmentedFile(ListFiles(input)); });
  }
  size_t nHeaderLen = fragmentedFiles.front().GetHeaderLen();
  if (any_of(fragmentedFiles.begin() + 1, fragmentedFiles.end(),
             [nHeaderLen](const auto &fragmentedFile) {
               return fragmentedFile.GetHeaderLen() != nHeaderLen;
             })) {
    spdlog::error("Input object headers are incompatible.");
    return -1;
  }

  if (output.storageType == BLOB) {
    auto destBlob = GetBlobClient(output).AsBlockBlobClient();
    vector<string> destBlockIds;
    Azure::Core::Http::HttpRange range;
    for (size_t nInputIndex = 0; nInputIndex != inputs.size(); nInputIndex++) {
      ostringstream oss;
      oss << setfill('0') << setw(64) << destBlockIds.size();
      string sBlockIdInBase10 = oss.str();
      vector<uint8_t> blockIdInBase10(sBlockIdInBase10.begin(),
                                      sBlockIdInBase10.end());
      string sBlockIdInBase64 =
          Azure::Core::Convert::Base64Encode(blockIdInBase10);
      destBlockIds.push_back(sBlockIdInBase64);

      range.Offset = nInputIndex == 0 ? 0 : nHeaderLen;
      Azure::Storage::Blobs::StageBlockFromUriOptions opts;
      opts.SourceRange = range;
      destBlob.StageBlockFromUri(sBlockIdInBase64,
                                 inputs[nInputIndex].azureUrl.GetAbsoluteUrl(),
                                 opts);
    }
    destBlob.CommitBlockList(destBlockIds);
  } else // SHARE
  {
    auto destFile = GetFileClient(output);
    destFile.Create(0LL);
    size_t nOffset = 0ULL;
    Azure::Core::Http::HttpRange range;
    for (size_t nInputIndex = 0; nInputIndex != inputs.size(); nInputIndex++) {
      range.Offset = nInputIndex == 0 ? 0 : nHeaderLen;
      destFile.UploadRangeFromUri(
          nOffset, inputs[nInputIndex].azureUrl.GetAbsoluteUrl(), range);
      nOffset += fragmentedFiles[nInputIndex].GetSize();
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
    spdlog::error("Caught an exception while performing basic URL parsing: URL "
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
    spdlog::error("URL {} contains invalid domain.", sUrl);
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
      spdlog::error("Undefined of empty environment variable: "
                    "AZURE_STORAGE_CONNECTION_STRING.");
      return -1;
    }
    smatch match;
    if (!regex_match(
            sPath, match,
            regex("([^/]+)/([^/]+)/(.+)"))) //  accountname/container/object  or
                                            //  accountname/container/object/
    {
      spdlog::error("Invalid emulated storage object path: {}.", sPath);
      return -1;
    }
    *result = std::move(
        ServiceRequest(url, bIsEmulatedStorage, BLOB, bDir,
                       BlobInfo{match[1].str(), match[2].str(), match[3].str()},
                       connectionStringCredential));
  } else if (bAzureBlobUrl) { // Real Azure cloud BLOB storage
    smatch match;
    if (!regex_match(
            sPath, match,
            regex("([^/]+)/(.+)"))) //  container/object  or  container/object/
    {
      spdlog::error("Invalid cloud blob path: {}.", sPath);
      return -1;
    }
    if (bConnectionStringDefined) {
      *result = std::move(
          ServiceRequest(url, bIsEmulatedStorage, BLOB, bDir,
                         BlobInfo{string(), match[1].str(), match[2].str()},
                         connectionStringCredential));
    } else {
      *result = std::move(
          ServiceRequest(url, bIsEmulatedStorage, BLOB, bDir,
                         BlobInfo{string(), match[1].str(), match[2].str()},
                         noConnectionStringCredential));
    }
  } else { // Real Azure cloud SHARE storage
    smatch match;
    if (!regex_match(
            sPath, match,
            regex("([^/]+)((?:/[^/]+)+/?)"))) //  share/path/to/a/file  or
                                              //  share/path/to/a/dir/
    {
      spdlog::error("Invalid cloud file path: {}.", sPath);
      return -1;
    }
    vector<string> fileOrDirPath = str::Split(match[2].str(), '/', -1, true);
    if (bConnectionStringDefined) {
      *result =
          std::move(ServiceRequest(url, bIsEmulatedStorage, SHARE, bDir,
                                   ShareInfo{match[1].str(), fileOrDirPath},
                                   connectionStringCredential));
    } else {
      *result =
          std::move(ServiceRequest(url, bIsEmulatedStorage, SHARE, bDir,
                                   ShareInfo{match[1].str(), fileOrDirPath},
                                   noConnectionStringCredential));
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
  spdlog::debug("Service URL is: {}.", result);
  return result;
}

string Driver::GetBlobContainerUrl(const ServiceRequest &request) const {
  ostringstream oss;
  oss << GetServiceUrl(request) << "/" << request.blob.sContainer;
  std::string result = oss.str();
  spdlog::debug("Blob container URL is: {}.", result);
  return result;
}

Azure::Storage::Blobs::BlobServiceClient
Driver::GetBlobServiceClient(const ServiceRequest &request) const {
  if (request.bUsingConnectionString) {
    return Azure::Storage::Blobs::BlobServiceClient(
        GetServiceUrl(request), request.connectionStringCredential);
  } else {
    return Azure::Storage::Blobs::BlobServiceClient(
        GetServiceUrl(request), request.noConnectionStringCredential);
  }
}

Azure::Storage::Blobs::BlobContainerClient
Driver::GetBlobContainerClient(const ServiceRequest &request) const {
  if (request.bUsingConnectionString) {
    return Azure::Storage::Blobs::BlobContainerClient(
        GetBlobContainerUrl(request), request.connectionStringCredential);
  } else {
    return Azure::Storage::Blobs::BlobContainerClient(
        GetBlobContainerUrl(request), request.noConnectionStringCredential);
  }
}

Azure::Storage::Blobs::BlobClient
Driver::GetBlobClient(const ServiceRequest &request) const {
  if (request.bUsingConnectionString) {
    return Azure::Storage::Blobs::BlobClient(
        request.azureUrl.GetAbsoluteUrl(), request.connectionStringCredential);
  } else {
    return Azure::Storage::Blobs::BlobClient(
        request.azureUrl.GetAbsoluteUrl(),
        request.noConnectionStringCredential);
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
  spdlog::debug("File share URL is: {}.", result);
  return result;
}

Azure::Storage::Files::Shares::ShareServiceClient
Driver::GetFileShareServiceClient(const ServiceRequest &request) const {
  if (request.bUsingConnectionString) {
    return Azure::Storage::Files::Shares::ShareServiceClient(
        GetServiceUrl(request), request.connectionStringCredential);
  } else {
    return Azure::Storage::Files::Shares::ShareServiceClient(
        GetServiceUrl(request), request.noConnectionStringCredential);
  }
}

Azure::Storage::Files::Shares::ShareClient
Driver::GetShareClient(const ServiceRequest &request) const {
  Azure::Storage::Files::Shares::ShareClientOptions opts;
  opts.ShareTokenIntent =
      Azure::Storage::Files::Shares::Models::ShareTokenIntent::Backup;
  if (request.bUsingConnectionString) {
    return Azure::Storage::Files::Shares::ShareClient(
        GetFileShareUrl(request), request.connectionStringCredential, opts);
  } else {
    return Azure::Storage::Files::Shares::ShareClient(
        GetFileShareUrl(request), request.noConnectionStringCredential, opts);
  }
}

Azure::Storage::Files::Shares::ShareDirectoryClient
Driver::GetDirClient(const ServiceRequest &request) const {
  return GetShareClient(request).GetRootDirectoryClient();
}

Azure::Storage::Files::Shares::ShareFileClient
Driver::GetFileClient(const ServiceRequest &request) const {
  Azure::Storage::Files::Shares::ShareClientOptions opts;
  opts.ShareTokenIntent =
      Azure::Storage::Files::Shares::Models::ShareTokenIntent::Backup;
  if (request.bUsingConnectionString) {
    return Azure::Storage::Files::Shares::ShareFileClient(
        request.azureUrl.GetAbsoluteUrl(), request.connectionStringCredential,
        opts);
  } else {
    return Azure::Storage::Files::Shares::ShareFileClient(
        request.azureUrl.GetAbsoluteUrl(), request.noConnectionStringCredential,
        opts);
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
      spdlog::error("Ancestor directory {}/{} does not exist.",
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
    spdlog::error("File stream not found.");
    return -1;
  }
  *result = it->second.get();
  return 0;
}
} // namespace az
