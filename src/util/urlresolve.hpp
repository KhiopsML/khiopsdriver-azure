#pragma once

#include <vector>
#include <queue>
#include <string>
#include <azure/storage/files/shares/share_directory_client.hpp>

namespace az
{
    using namespace std;

    vector<string> ResolveUrlRecursively(
        const Azure::Storage::Files::Shares::ShareDirectoryClient& dirClient,
        queue<string> urlPathSegments,
        bool bLookingForDirs,
        const string& sPath
    );
}
