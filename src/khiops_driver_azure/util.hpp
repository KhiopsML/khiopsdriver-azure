/*
A collection of utilities, specific to the usage of the Azure cloud storage services.
*/

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <azure/core.hpp>
#include <azure/core/http/curl_transport.hpp>
#include <azure/storage/blobs/blob_client.hpp>
#include <azure/storage/blobs/blob_container_client.hpp>
#include <azure/storage/blobs/blob_service_client.hpp>
#include <azure/storage/files/shares/share_client.hpp>
#include <azure/storage/files/shares/share_directory_client.hpp>
#include <azure/storage/files/shares/share_file_client.hpp>
#include <azure/storage/files/shares/share_service_client.hpp>
#include <azure/storage/common/storage_credential.hpp>


namespace khiops_driver_azure {

int AzureUrlFromString(Azure::Core::Url *result, const std::string &url);

enum StorageType { BLOB, FILE_SHARE };
int StorageTypeFromHost(StorageType *result, const std::string &host);

int EmulatedBlobPathFromString(std::string *account_name, std::string *blob_container, std::string *blob, const std::string &path);
int BlobPathFromString(std::string *blob_container, std::string *blob, const std::string &path);
int FileSharePathFromString(std::string *file_share, std::vector<std::string> *file_path, const std::string &path);

int ResolveFragmentUrls(std::vector<std::string> *result, StorageType storage_type, const Azure::Core::Url &azure_url);
int ResolveFragmentUrlsCheckNotEmpty(std::vector<std::string> *result, StorageType storage_type, const Azure::Core::Url &azure_url);

std::string BuildEmulatedServiceUrl(const std::string &scheme, const std::string &host, uint16_t port, const std::string &account_name);
std::string BuildEmulatedServiceUrl(const Azure::Core::Url &azure_url, const std::string &account_name);
std::string BuildServiceUrl(const std::string &scheme, const std::string &host, uint16_t port);
std::string BuildServiceUrl(const Azure::Core::Url &azure_url);

std::string BuildBlobContainerUrl(const std::string &service_url, const std::string &container);
Azure::Storage::Blobs::BlobContainerClient GetBlobContainerClient(const std::string &service_url, const std::string &container);
std::vector<std::string> ListBlobs(const std::string &service_url, const std::string &container, const std::string &blob);
int ListBlobsCheckNotEmpty(std::vector<std::string> *result, const std::string &service_url, const std::string &container, const std::string &blob);
int GetBlobClient(Azure::Storage::Blobs::BlobClient *result, const std::string &url);

std::string GetFileShareUrl(const std::string &service_url, const std::string &file_share);
Azure::Storage::Files::Shares::ShareClient GetShareClient(const std::string &service_url, const std::string &file_share);
Azure::Storage::Files::Shares::ShareDirectoryClient GetRootDirClient(const std::string &service_url, const std::string &file_share);
Azure::Storage::Files::Shares::ShareDirectoryClient GetDirClient(const std::string &url);
std::vector<std::string> ListDirs(const std::string &service_url, const std::string &file_share, const std::vector<std::string> &file_path);
std::vector<std::string> ListFiles(const std::string &service_url, const std::string &file_share, const std::vector<std::string> &file_path);
int ListFilesCheckNotEmpty(std::vector<std::string> *result, const std::string &url, const std::string &file_share, const std::vector<std::string> &file_path);
int GetParentDir(Azure::Storage::Files::Shares::ShareDirectoryClient *result, const std::string &service_url, const std::string &file_share, const std::vector<std::string> &file_path);
int GetFileClient(Azure::Storage::Files::Shares::ShareFileClient *result, const std::string &url);

int ReadFragment(std::string *result, bool *stopped_on_termchar, StorageType storage_type, const std::string &fragment_url, const Azure::ETag &version, size_t offset, size_t maxlength);
int ReadFragment(std::string *result, bool *stopped_on_termchar, StorageType storage_type, const std::string &fragment_url, const Azure::ETag &version, size_t offset, size_t maxlength, char termchar);
int ReadFragmentToBuffer(size_t *result, StorageType storage_type, const std::string &fragment_url, const Azure::ETag &version, size_t offset, size_t maxlength, void *buffer);

bool parse_globbing_pattern(const std::string &pattern, std::string *prefix, std::string *suffix);

}