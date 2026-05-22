#pragma once

#include <string>
#include <vector>
#include <azure/storage/blobs/blob_client.hpp>
#include <azure/storage/blobs/blob_container_client.hpp>
#include <azure/storage/blobs/blob_service_client.hpp>
#include <azure/storage/files/shares/share_client.hpp>
#include <azure/storage/files/shares/share_directory_client.hpp>
#include <azure/storage/files/shares/share_file_client.hpp>
#include <azure/storage/files/shares/share_service_client.hpp>
#include "khiops_driver_azure/servicerequest.hpp"

namespace khiops_driver_azure {

bool IsEmulatedStorage();

int ParseUrl(ServiceRequest *result, const std::string &sUrl);

std::string GetServiceUrl(const ServiceRequest &request);

std::string GetBlobContainerUrl(const ServiceRequest &request);
Azure::Storage::Blobs::BlobContainerClient GetBlobContainerClient(const ServiceRequest &request);
std::vector<Azure::Storage::Blobs::BlobClient> ListBlobs(const ServiceRequest &request);

std::string GetFileShareUrl(const ServiceRequest &request);
Azure::Storage::Files::Shares::ShareClient GetShareClient(const ServiceRequest &request);
Azure::Storage::Files::Shares::ShareDirectoryClient GetDirClient(const ServiceRequest &request);
std::vector<Azure::Storage::Files::Shares::ShareDirectoryClient> ListDirs(const ServiceRequest &request);
std::vector<Azure::Storage::Files::Shares::ShareFileClient> ListFiles(const ServiceRequest &request);

}