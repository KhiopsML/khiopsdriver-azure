// Functions relative to share file path resolution.
// They are used to list all share files matching a given URL.

#pragma once

#include <vector>
#include <queue>
#include <string>
#include <azure/storage/files/shares/share_directory_client.hpp>
#include <azure/storage/files/shares/share_file_client.hpp>

namespace az
{
    std::vector<Azure::Storage::Files::Shares::ShareDirectoryClient> ResolveDirsPathRecursively(
        const Azure::Storage::Files::Shares::ShareDirectoryClient& dirClient,
        std::queue<std::string> pathSegments
    );

    std::vector<Azure::Storage::Files::Shares::ShareFileClient> ResolveFilesPathRecursively(
        const Azure::Storage::Files::Shares::ShareDirectoryClient& dirClient,
        std::queue<std::string> pathSegments
    );
}
