/*
A collection of utilities, specific to the usage of the Azure cloud storage services.
*/

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <azure/core.hpp>
#include <azure/storage/blobs/blob_client.hpp>
#include <azure/storage/blobs/blob_container_client.hpp>
#include <azure/storage/blobs/blob_service_client.hpp>
#include <azure/storage/files/shares/share_client.hpp>
#include <azure/storage/files/shares/share_directory_client.hpp>
#include <azure/storage/files/shares/share_file_client.hpp>
#include <azure/storage/files/shares/share_service_client.hpp>
#include <azure/storage/common/storage_credential.hpp>
#include "khiops_driver_common/backend.hpp"


namespace khiops_driver_azure {

enum StorageType { BLOB, FILE_SHARE };
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

struct ServiceRequest {
    Azure::Core::Url azure_url;
    bool is_dir;
    bool is_emulated_storage;
    StorageType storage_type;
    ObjectPath object_path;
    bool is_using_connection_string;
    std::shared_ptr<Azure::Storage::StorageSharedKeyCredential> connection_string_credential;
    std::shared_ptr<Azure::Core::Credentials::TokenCredential> no_connection_string_credential;
};

bool IsEmulatedStorage();

int BuildServiceRequest(std::unique_ptr<ServiceRequest> *result, const std::string &url);

void LogServiceRequest(const ServiceRequest &request);

std::string GetServiceUrl(const ServiceRequest &request);

std::string GetBlobContainerUrl(const ServiceRequest &request);
Azure::Storage::Blobs::BlobContainerClient GetBlobContainerClient(const ServiceRequest &request);
std::vector<std::string> ListBlobs(const ServiceRequest &request);
int GetBlobClient(Azure::Storage::Blobs::BlobClient *result, const ServiceRequest &request, const std::string &url);
int GetBlobClient(Azure::Storage::Blobs::BlobClient *result, const ServiceRequest &request);

std::string GetFileShareUrl(const ServiceRequest &request);
Azure::Storage::Files::Shares::ShareClient GetShareClient(const ServiceRequest &request);
Azure::Storage::Files::Shares::ShareDirectoryClient GetDirClient(const ServiceRequest &request, const std::string &url);
Azure::Storage::Files::Shares::ShareDirectoryClient GetDirClient(const ServiceRequest &request);
std::vector<std::string> ListDirs(const ServiceRequest &request);
std::vector<std::string> ListFiles(const ServiceRequest &request);
int GetParentDir(Azure::Storage::Files::Shares::ShareDirectoryClient *result, const ServiceRequest &request);
int GetFileClient(Azure::Storage::Files::Shares::ShareFileClient *result, const ServiceRequest &request, const std::string &url);
int GetFileClient(Azure::Storage::Files::Shares::ShareFileClient *result, const ServiceRequest &request);

std::vector<std::string> ListBlobsOrFiles(const ServiceRequest &request);
int ListBlobsOrFilesCheckNotEmpty(std::vector<std::string> *result, const ServiceRequest &request);

}