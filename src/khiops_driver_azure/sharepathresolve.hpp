// Functions relative to share file path resolution.
// They are used to list all share files matching a given URL.

#pragma once

#include <azure/storage/files/shares/share_directory_client.hpp>
#include <azure/storage/files/shares/share_file_client.hpp>
#include <queue>
#include <string>
#include <vector>

namespace khiops_driver_azure {
std::vector<std::string>
ResolveDirsPathRecursively(
    const Azure::Storage::Files::Shares::ShareDirectoryClient &dirClient,
    std::queue<std::string> pathSegments);

std::vector<std::string>
ResolveFilesPathRecursively(
    const Azure::Storage::Files::Shares::ShareDirectoryClient &dirClient,
    std::queue<std::string> pathSegments);
} // namespace khiops_driver_azure
