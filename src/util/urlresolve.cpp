#include "urlresolve.hpp"
#include <regex>
#include "glob.hpp"

namespace az
{
    vector<string> ResolveUrlRecursively(
        const Azure::Storage::Files::Shares::ShareDirectoryClient& dirClient,
        queue<string> urlPathSegments,
        bool bLookingForDirs,
        const string& sPath
    )
    {
        if (urlPathSegments.empty())
        {
            return {};
        }
        const string sUrlPathSegment = urlPathSegments.front();
        urlPathSegments.pop();
        if (sUrlPathSegment == "**")
        {
            if (!bLookingForDirs && urlPathSegments.empty())
            {
                return ResolveDoubleStarAsFiles(dirClient, sPath);
            }
            else
            {
                return ResolveDoubleStar(dirClient, urlPathSegments, bLookingForDirs, sPath);
            }
        }
        else if (sUrlPathSegment.find_first_of("?[*") != string::npos)
        {
            return ResolveRegex(dirClient, urlPathSegments, bLookingForDirs, sUrlPathSegment, sPath);
        }
        else
        {
            return ResolveRaw(dirClient, urlPathSegments, bLookingForDirs, sUrlPathSegment, sPath);
        }
    }

    static vector<string> ResolveDoubleStar(
        const Azure::Storage::Files::Shares::ShareDirectoryClient& dirClient,
        queue<string> urlPathSegments,
        bool bLookingForDirs,
        const string& sPath
    )
    {
        vector<string> result = ResolveUrlRecursively(dirClient, urlPathSegments, bLookingForDirs, sPath);
        vector<string> subresult;
        for (auto pagedFileAndDirList = dirClient.ListFilesAndDirectories(); pagedFileAndDirList.HasPage(); pagedFileAndDirList.MoveToNextPage())
        {
            for (const auto& dirItem : pagedFileAndDirList.Directories)
            {
                subresult = ResolveDoubleStar(dirClient.GetSubdirectoryClient(dirItem.Name), urlPathSegments, bLookingForDirs, sPath + "/" + dirItem.Name);
                result.insert(result.end(), subresult.begin(), subresult.end());
            }
        }
        return result;
    }

    static vector<string> ResolveDoubleStarAsFiles(
        const Azure::Storage::Files::Shares::ShareDirectoryClient& dirClient,
        const string& sPath
    )
    {
        vector<string> result, subresult;
        for (auto pagedFileAndDirList = dirClient.ListFilesAndDirectories(); pagedFileAndDirList.HasPage(); pagedFileAndDirList.MoveToNextPage())
        {
            for (const auto& fileItem : pagedFileAndDirList.Files)
            {
                result.emplace_back(sPath + "/" + fileItem.Name);
            }
            for (const auto& dirItem : pagedFileAndDirList.Directories)
            {
                subresult = ResolveDoubleStarAsFiles(dirClient.GetSubdirectoryClient(dirItem.Name), sPath + "/" + dirItem.Name);
                result.insert(result.end(), subresult.begin(), subresult.end());
            }
        }
        return result;
    }

    static vector<string> ResolveRegex(
        const Azure::Storage::Files::Shares::ShareDirectoryClient& dirClient,
        queue<string> urlPathSegments,
        bool bLookingForDirs,
        const string& sGlobbingPattern,
        const string& sPath
    )
    {
        regex re = globbing::RegexFromGlobbingPattern(sGlobbingPattern);
        if (urlPathSegments.empty())
        {
            if (bLookingForDirs)
            {
                return FindDirsByRegex(dirClient, sPath, re);
            }
            else
            {
                return FindFilesByRegex(dirClient, sPath, re);
            }
        }
        else
        {
            vector<string> result, subresult;
            for (auto pagedFileAndDirList = dirClient.ListFilesAndDirectories(); pagedFileAndDirList.HasPage(); pagedFileAndDirList.MoveToNextPage())
            {
                for (const auto& dirItem : pagedFileAndDirList.Directories)
                {
                    if (regex_match(dirItem.Name, re))
                    {
                        subresult = ResolveUrlRecursively(dirClient.GetSubdirectoryClient(dirItem.Name), urlPathSegments, bLookingForDirs, sPath + "/" + dirItem.Name);
                        result.insert(result.end(), subresult.begin(), subresult.end());
                    }
                }
            }
            return result;
        }
    }

    static vector<string> ResolveRaw(
        const Azure::Storage::Files::Shares::ShareDirectoryClient& dirClient,
        queue<string> urlPathSegments,
        bool bLookingForDirs,
        const string& sName,
        const string& sPath
    )
    {
        if (urlPathSegments.empty())
        {
            if (bLookingForDirs)
            {
                return FindDirsByName(dirClient, sPath, sName);
            }
            else
            {
                return FindFilesByName(dirClient, sPath, sName);
            }
        }
        else
        {
            for (auto pagedFileAndDirList = dirClient.ListFilesAndDirectories(); pagedFileAndDirList.HasPage(); pagedFileAndDirList.MoveToNextPage())
            {
                for (const auto& dirItem : pagedFileAndDirList.Directories)
                {
                    if (dirItem.Name == sName)
                    {
                        return ResolveUrlRecursively(dirClient.GetSubdirectoryClient(dirItem.Name), urlPathSegments, bLookingForDirs, sPath + "/" + dirItem.Name);
                    }
                }
            }
            return {};
        }
    }

    static vector<string> FindDirs(
        const Azure::Storage::Files::Shares::ShareDirectoryClient& dirClient,
        const string& sPath,
        const function<bool(const Azure::Storage::Files::Shares::Models::DirectoryItem&)>& predicate
    )
    {
        vector<string> result;
        for (auto pagedFileAndDirList = dirClient.ListFilesAndDirectories(); pagedFileAndDirList.HasPage(); pagedFileAndDirList.MoveToNextPage())
        {
            for (const auto& dirItem : pagedFileAndDirList.Directories)
            {
                if (predicate(dirItem))
                {
                    result.emplace_back(sPath + "/" + dirItem.Name);
                }
            }
        }
        return result;
    }

    static vector<string> FindFiles(
        const Azure::Storage::Files::Shares::ShareDirectoryClient& dirClient,
        const string& sPath,
        const function<bool(const Azure::Storage::Files::Shares::Models::FileItem&)>& predicate
    )
    {
        vector<string> result;
        for (auto pagedFileAndDirList = dirClient.ListFilesAndDirectories(); pagedFileAndDirList.HasPage(); pagedFileAndDirList.MoveToNextPage())
        {
            for (const auto& fileItem : pagedFileAndDirList.Files)
            {
                if (predicate(fileItem))
                {
                    result.emplace_back(sPath + "/" + fileItem.Name);
                }
            }
        }
        return result;
    }

    static vector<string> FindDirsByName(
        const Azure::Storage::Files::Shares::ShareDirectoryClient& dirClient,
        const string& sPath,
        const string& sName
    )
    {
        return FindDirs(dirClient, sPath, [sName](const Azure::Storage::Files::Shares::Models::DirectoryItem& dirItem) { return dirItem.Name == sName; });
    }

    static vector<string> FindDirsByRegex(
        const Azure::Storage::Files::Shares::ShareDirectoryClient& dirClient,
        const string& sPath, 
        const regex& re
    )
    {
        return FindDirs(dirClient, sPath, [re](const Azure::Storage::Files::Shares::Models::DirectoryItem& dirItem) { return regex_match(dirItem.Name, re); });
    }

    static vector<string> FindFilesByName(
        const Azure::Storage::Files::Shares::ShareDirectoryClient& dirClient,
        const string& sPath, 
        const string& sName
    )
    {
        return FindFiles(dirClient, sPath, [sName](const Azure::Storage::Files::Shares::Models::FileItem& fileItem) { return fileItem.Name == sName; });
    }

    static vector<string> FindFilesByRegex(
        const Azure::Storage::Files::Shares::ShareDirectoryClient& dirClient,
        const string& sPath, 
        const regex& re
    )
    {
        return FindFiles(dirClient, sPath, [re](const Azure::Storage::Files::Shares::Models::FileItem& fileItem) { return regex_match(fileItem.Name, re); });
    }
}
