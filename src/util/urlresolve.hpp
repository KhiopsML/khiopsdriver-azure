#pragma once

#include <vector>
#include <queue>
#include <string>
#include <azure/storage/files/shares/share_directory_client.hpp>
#include <azure/storage/files/shares/share_file_client.hpp>

namespace az
{
    std::vector<Azure::Storage::Files::Shares::ShareDirectoryClient> ResolveDirsUrlRecursively(
        const Azure::Storage::Files::Shares::ShareDirectoryClient& dirClient,
        std::queue<std::string> urlPathSegments
    );

    std::vector<Azure::Storage::Files::Shares::ShareFileClient> ResolveFilesUrlRecursively(
        const Azure::Storage::Files::Shares::ShareDirectoryClient& dirClient,
        std::queue<std::string> urlPathSegments
    );
}
