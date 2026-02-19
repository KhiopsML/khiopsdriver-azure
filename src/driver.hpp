// The high-level driver, performing most of the work.

#pragma once

namespace az {
struct BlobInfo;
struct ShareInfo;
struct ServiceRequest;
class Driver;
} // namespace az

#include "azureplugin.hpp"
#include "filestream.hpp"
#include "util.hpp"
#include <azure/storage/blobs/blob_client.hpp>
#include <azure/storage/blobs/blob_container_client.hpp>
#include <azure/storage/blobs/blob_service_client.hpp>
#include <azure/storage/files/shares/share_client.hpp>
#include <azure/storage/files/shares/share_directory_client.hpp>
#include <azure/storage/files/shares/share_file_client.hpp>
#include <azure/storage/files/shares/share_service_client.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace az {
static constexpr long long int DEFAULT_PREFERRED_BUFFER_SIZE =
    4LL * 1024LL * 1024LL;

class Driver {
public:
  Driver();
  ~Driver();

  size_t GetPreferredBufferSize() const;

  int Exists(bool *result, const std::string &sUrl) const;
  int GetSize(size_t *result, const std::string &sUrl) const;
  int OpenForReading(FileStream **result, const std::string &sUrl);
  int OpenForWriting(FileStream **result, const std::string &sUrl);
  int OpenForAppending(FileStream **result, const std::string &sUrl);
  int Close(void *handle);
  int Read(size_t *nRead, void *handle, void *dest, size_t nSize,
           size_t nCount);
  int Seek(void *handle, long long int nOffset, int nOrigin);
  void GetLastError(std::string **result);
  int Write(size_t *nWritten, void *handle, const void *source, size_t nSize,
            size_t nCount);
  int Flush(void *handle);
  int Remove(const std::string &sUrl) const;
  int MkDir(const std::string &sUrl) const;
  int RmDir(const std::string &sUrl) const;
  int GetFreeDiskSpace(size_t *result) const;
  int CopyTo(const std::string &sUrl, const std::string &destUrl);
  int CopyFrom(const std::string &sUrl, const std::string &sourceUrl);
  int Concatenate(const std::vector<std::string> &inputUrls,
                  const std::string &sDestUrl);

private:
  bool IsEmulatedStorage() const;

  int ParseUrl(ServiceRequest *result, const std::string &sUrl) const;

  /*** Generic URL Retrieval ***/
  std::string GetServiceUrl(const ServiceRequest &request) const;

  /*** Blob URL retrieval ***/
  std::string GetBlobContainerUrl(const ServiceRequest &request) const;
  Azure::Storage::Blobs::BlobServiceClient
  GetBlobServiceClient(const ServiceRequest &request) const;
  Azure::Storage::Blobs::BlobContainerClient
  GetBlobContainerClient(const ServiceRequest &request) const;
  Azure::Storage::Blobs::BlobClient
  GetBlobClient(const ServiceRequest &request) const;
  std::vector<Azure::Storage::Blobs::BlobClient>
  ListBlobs(const ServiceRequest &request) const;

  /*** File URL Retrieval ***/
  std::string GetFileShareUrl(const ServiceRequest &request) const;
  Azure::Storage::Files::Shares::ShareServiceClient
  GetFileShareServiceClient(const ServiceRequest &request) const;
  Azure::Storage::Files::Shares::ShareClient
  GetShareClient(const ServiceRequest &request) const;
  Azure::Storage::Files::Shares::ShareDirectoryClient
  GetDirClient(const ServiceRequest &request) const;
  Azure::Storage::Files::Shares::ShareFileClient
  GetFileClient(const ServiceRequest &request) const;
  std::vector<Azure::Storage::Files::Shares::ShareDirectoryClient>
  ListDirs(const ServiceRequest &request) const;
  std::vector<Azure::Storage::Files::Shares::ShareFileClient>
  ListFiles(const ServiceRequest &request) const;
  int GetParentDir(Azure::Storage::Files::Shares::ShareDirectoryClient *result,
                   const ServiceRequest &request) const;

  /*** File Stream Management ***/
  FileStream *RegisterFileStream(FileStream &&fileStream);
  int RetrieveFileStream(FileStream **result, void *handle) const;
  std::unordered_map<void *, std::unique_ptr<FileStream>> fileStreams;

  size_t nPreferredBufferSize;
};
} // namespace az
