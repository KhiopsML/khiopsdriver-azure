#include "urlresolve.hpp"
#include <algorithm>
#include <iterator>
#include <functional>
#include "glob.hpp"
#include "../exception.hpp"
#include "../contrib/globmatch.hpp"
#include "string.hpp"

namespace az
{
    vector<string> ResolveUrlRecursively(
        const Azure::Storage::Files::Shares::ShareDirectoryClient& dirClient,
        queue<string> urlPathSegments,
        bool bLookingForDirs,
        const string& sPath
    )
    {
        try
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
                return ResolveGlobbing(dirClient, urlPathSegments, bLookingForDirs, sUrlPathSegment, sPath);
            }
            else
            {
                return ResolveRaw(dirClient, urlPathSegments, bLookingForDirs, sUrlPathSegment, sPath);
            }
        }
        catch (const Azure::Core::Http::TransportException& exc)
        {
            throw NetworkError();
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

    static vector<string> ResolveGlobbing(
        const Azure::Storage::Files::Shares::ShareDirectoryClient& dirClient,
        queue<string> urlPathSegments,
        bool bLookingForDirs,
        const string& sGlobbingPattern,
        const string& sPath
    )
    {
        if (urlPathSegments.empty())
        {
            if (bLookingForDirs)
            {
                return FindDirsByGlob(dirClient, sPath, sGlobbingPattern);
            }
            else
            {
                return FindFilesByGlob(dirClient, sPath, sGlobbingPattern);
            }
        }
        else
        {
            vector<string> result, subresult;
            for (auto pagedFileAndDirList = dirClient.ListFilesAndDirectories(); pagedFileAndDirList.HasPage(); pagedFileAndDirList.MoveToNextPage())
            {
                for (const auto& dirItem : pagedFileAndDirList.Directories)
                {
                    if (globbing::GitignoreGlobMatch(dirItem.Name, sGlobbingPattern))
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
                return FindDirsByName(dirClient, sName);
            }
            else
            {
                return FindFilesByName(dirClient, sName);
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

    static vector<Azure::Storage::Files::Shares::ShareDirectoryClient> ResolveDirsRaw(
        const Azure::Storage::Files::Shares::ShareDirectoryClient& dirClient,
        queue<string> urlPathSegments,
        const string& sName,
        const string& sPath
    )
    {
        if (urlPathSegments.empty())
        {
            return FindDirsByName(dirClient, sName);
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

    static vector<Azure::Storage::Files::Shares::ShareDirectoryClient> FindDirs(
        const Azure::Storage::Files::Shares::ShareDirectoryClient& dirClient,
        const function<bool(const Azure::Storage::Files::Shares::Models::DirectoryItem&)>& predicate,
        const string& prefix
    )
    {
        Azure::Storage::Files::Shares::ListFilesAndDirectoriesOptions opts;
        opts.Prefix = prefix;
        vector<Azure::Storage::Files::Shares::ShareDirectoryClient> result;
        for (auto pagedFileAndDirList = dirClient.ListFilesAndDirectories(opts); pagedFileAndDirList.HasPage(); pagedFileAndDirList.MoveToNextPage())
        {
            for (const auto& dirItem : pagedFileAndDirList.Directories)
            {
                if (predicate(dirItem))
                {
                    result.push_back(dirClient.GetSubdirectoryClient(dirItem.Name));
                }
            }
        }
        return result;
    }

    static vector<Azure::Storage::Files::Shares::ShareFileClient> FindFiles(
        const Azure::Storage::Files::Shares::ShareDirectoryClient& dirClient,
        const function<bool(const Azure::Storage::Files::Shares::Models::FileItem&)>& predicate,
        const string& prefix
    )
    {
        Azure::Storage::Files::Shares::ListFilesAndDirectoriesOptions opts;
        opts.Prefix = prefix;
        vector<Azure::Storage::Files::Shares::ShareFileClient> result;
        for (auto pagedFileAndDirList = dirClient.ListFilesAndDirectories(opts); pagedFileAndDirList.HasPage(); pagedFileAndDirList.MoveToNextPage())
        {
            for (const auto& fileItem : pagedFileAndDirList.Files)
            {
                if (predicate(fileItem))
                {
                    result.push_back(dirClient.GetFileClient(fileItem.Name));
                }
            }
        }
        return result;
    }

    template<typename ItemT>
    static bool ItemHasName(const ItemT& item, const string& sName)
    {
        return item.Name == sName;
    }

    template<typename ItemT>
    static bool ItemNameMatches(const ItemT& item, const string& sGlob)
    {
        return globbing::GitignoreGlobMatch(item.Name, sGlob);
    }

    static string PrefixFromName(const string& sName)
    {
        return sName;
    }

    static string PrefixFromGlob(const string& sGlob)
    {
        return sGlob.substr(0, globbing::FindGlobbingChar(sGlob));
    }

    static vector<Azure::Storage::Files::Shares::ShareDirectoryClient> FindDirsByName(
        const Azure::Storage::Files::Shares::ShareDirectoryClient& dirClient, const string& sName
    )
    {
        return FindDirs(dirClient, [sName](const Azure::Storage::Files::Shares::Models::DirectoryItem& item) { return ItemHasName(item, sName); }, PrefixFromName(sName));
    }

    static vector<Azure::Storage::Files::Shares::ShareDirectoryClient> FindDirsByGlob(
        const Azure::Storage::Files::Shares::ShareDirectoryClient& dirClient, const string& sGlob
    )
    {
        return FindDirs(dirClient, [sGlob](const Azure::Storage::Files::Shares::Models::DirectoryItem& item) { return ItemNameMatches(item, sGlob); }, PrefixFromGlob(sGlob));
    }

    static vector<Azure::Storage::Files::Shares::ShareFileClient> FindFilesByName(
        const Azure::Storage::Files::Shares::ShareDirectoryClient& dirClient, const string& sName
    )
    {
        return FindFiles(dirClient, [sName](const Azure::Storage::Files::Shares::Models::FileItem& item) { return ItemHasName(item, sName); }, PrefixFromName(sName));
    }

    static vector<Azure::Storage::Files::Shares::ShareFileClient> FindFilesByGlob(
        const Azure::Storage::Files::Shares::ShareDirectoryClient& dirClient, const string& sGlob
    )
    {
        return FindFiles(dirClient, [sGlob](const Azure::Storage::Files::Shares::Models::FileItem& item) { return ItemNameMatches(item, sGlob); }, PrefixFromGlob(sGlob));
    }
}
