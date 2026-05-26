#pragma once

#include <string>
#include <vector>
#include <memory>
#include <azure/storage/blobs/blob_client.hpp>
#include <azure/storage/blobs/blob_container_client.hpp>
#include <azure/storage/blobs/blob_service_client.hpp>
#include <azure/storage/files/shares/share_client.hpp>
#include <azure/storage/files/shares/share_directory_client.hpp>
#include <azure/storage/files/shares/share_file_client.hpp>
#include <azure/storage/files/shares/share_service_client.hpp>
#include "khiops_driver_azure/servicerequest.hpp"

namespace khiops_driver_azure {

enum StorageType { BLOB, SHARE };
int StorageTypeOfUrl(StorageType *result, const Azure::Core::Url &url, bool is_emulated_storage);

struct ObjectPath {
    std::unique_ptr<std::string> emulated_account_name;
    std::unique_ptr<std::string> blob_container;
    std::unique_ptr<std::string> blob;
    std::unique_ptr<std::string> file_share;
    std::unique_ptr<std::vector<std::string>> file_path;
};
std::string ObjectPathToString(const ObjectPath &object_path);
int ObjectPathOfUrl(ObjectPath *result, const Azure::Core::Url &url, bool is_emulated_storage, StorageType storage_type);

bool IsEmulatedStorage();

int BuildServiceRequest(ServiceRequest *result, const std::string &url);

std::string GetServiceUrl(const ServiceRequest &request);

std::string GetBlobContainerUrl(const ServiceRequest &request);
Azure::Storage::Blobs::BlobContainerClient GetBlobContainerClient(const ServiceRequest &request);
std::vector<std::string> ListBlobs(const ServiceRequest &request);
Azure::Storage::Blobs::BlobClient GetBlobClient(const ServiceRequest &request);

std::string GetFileShareUrl(const ServiceRequest &request);
Azure::Storage::Files::Shares::ShareClient GetShareClient(const ServiceRequest &request);
Azure::Storage::Files::Shares::ShareDirectoryClient GetDirClient(const ServiceRequest &request);
std::vector<std::string> ListDirs(const ServiceRequest &request);
std::vector<std::string> ListFiles(const ServiceRequest &request);
int GetParentDir(Azure::Storage::Files::Shares::ShareDirectoryClient *result, const ServiceRequest &request);
Azure::Storage::Files::Shares::ShareFileClient GetFileClient(const ServiceRequest &request);

}