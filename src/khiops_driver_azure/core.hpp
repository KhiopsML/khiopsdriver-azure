#pragma once

#include <string>
#include "khiops_driver_azure/util.hpp"

namespace khiops_driver_azure {

int GetSystemPreferredBufferSize(size_t *result);

int Remove(const std::vector<std::string> &fragment_urls, StorageType storage_type);

int ConcatBlob(const std::string &dest_url, const std::vector<std::string> &source_urls, const std::vector<std::string> &source_blob_containers, const std::vector<std::string> &source_blobs);
int ConcatFileShare(const std::string &dest_url, const std::vector<std::string> &source_urls, const std::vector<std::string> &source_file_shares, const std::vector<std::vector<std::string>> &source_file_paths);

}