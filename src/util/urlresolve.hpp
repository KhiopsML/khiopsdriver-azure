#pragma once

#include <vector>
#include <queue>
#include <string>
#include <azure/storage/files/shares/share_directory_client.hpp>

namespace az
{
    using namespace std;

    vector<Azure::Storage::Files::Shares::Models::DirectoryItem> ListDirsRecursively(
        const Azure::Storage::Files::Shares::ShareDirectoryClient& dirClient,
        queue<string> urlPathSegments,
        const string& sPath
    );

    vector<Azure::Storage::Files::Shares::Models::FileItem> ListFilesRecursively(
        const Azure::Storage::Files::Shares::ShareDirectoryClient& dirClient,
        queue<string> urlPathSegments,
        const string& sPath
    );
}
