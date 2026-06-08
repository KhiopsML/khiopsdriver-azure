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
#include "khiops_driver_common/remote_object_request.hpp"
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

struct RemoteObjectRequestUserData {
    std::string service_url;
    bool is_emulated_storage;
    StorageType storage_type;
    ObjectPath object_path;
    bool is_using_connection_string;
    std::shared_ptr<Azure::Storage::StorageSharedKeyCredential> connection_string_credential;
    std::shared_ptr<Azure::Core::Credentials::TokenCredential> no_connection_string_credential;
};

const RemoteObjectRequestUserData *GetUserData(const RemoteObjectRequest &request);

struct FileWriterUserData {
    // Used only for blob storage
    unique_ptr<vector<string>> block_ids = nullptr;
    // Used only for blob storage
    unique_ptr<Azure::Storage::Blobs::BlobClient> blob_client;
    // Used only for file share storage
    unique_ptr<Azure::Storage::Files::Shares::ShareFileClient> share_file_client;
};

bool IsEmulatedStorage();

std::string GetBlobContainerUrl(const RemoteObjectRequest &request);
Azure::Storage::Blobs::BlobContainerClient GetBlobContainerClient(const RemoteObjectRequest &request);
std::vector<std::string> ListBlobs(const RemoteObjectRequest &request);
int GetBlobClient(Azure::Storage::Blobs::BlobClient *result, const RemoteObjectRequest &request, const std::string &url);
int GetBlobClient(Azure::Storage::Blobs::BlobClient *result, const RemoteObjectRequest &request);

std::string GetFileShareUrl(const RemoteObjectRequest &request);
Azure::Storage::Files::Shares::ShareClient GetShareClient(const RemoteObjectRequest &request);
Azure::Storage::Files::Shares::ShareDirectoryClient GetDirClient(const RemoteObjectRequest &request, const std::string &url);
Azure::Storage::Files::Shares::ShareDirectoryClient GetDirClient(const RemoteObjectRequest &request);
std::vector<std::string> ListDirs(const RemoteObjectRequest &request);
std::vector<std::string> ListFiles(const RemoteObjectRequest &request);
int GetParentDir(Azure::Storage::Files::Shares::ShareDirectoryClient *result, const RemoteObjectRequest &request);
int GetFileClient(Azure::Storage::Files::Shares::ShareFileClient *result, const RemoteObjectRequest &request, const std::string &url);
int GetFileClient(Azure::Storage::Files::Shares::ShareFileClient *result, const RemoteObjectRequest &request);

std::vector<std::string> ListBlobsOrFiles(const RemoteObjectRequest &request);
int ListBlobsOrFilesCheckNotEmpty(std::vector<std::string> *result, const RemoteObjectRequest &request);

}