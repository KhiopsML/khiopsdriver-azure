#include "sharepathresolve.hpp"
#include <functional>
#include "util/glob.hpp"
#include "exception.hpp"
#include "contrib/globmatch.hpp"

namespace az
{
    using ShareDirectoryClient = Azure::Storage::Files::Shares::ShareDirectoryClient;
    using ShareFileClient = Azure::Storage::Files::Shares::ShareFileClient;
    using DirectoryItem = Azure::Storage::Files::Shares::Models::DirectoryItem;
    using FileItem = Azure::Storage::Files::Shares::Models::FileItem;
    using ListFilesAndDirectoriesPagedResponse = Azure::Storage::Files::Shares::ListFilesAndDirectoriesPagedResponse;
    using ListFilesAndDirectoriesOptions = Azure::Storage::Files::Shares::ListFilesAndDirectoriesOptions;

    vector<ShareDirectoryClient> ResolveDirsPathRecursively(const ShareDirectoryClient& dirClient, queue<string> pathSegments);

    vector<ShareFileClient> ResolveFilesPathRecursively(const ShareDirectoryClient& dirClient, queue<string> pathSegments);

    template<
        typename ClientT,
        vector<ClientT>(*ResolvePathRecursively)(const ShareDirectoryClient&, queue<string>)
    >
    static vector<ClientT> ResolveDoubleStar(const ShareDirectoryClient& dirClient, queue<string> pathSegments);

    static vector<ShareFileClient> ResolveFilesDoubleStar(const ShareDirectoryClient& dirClient);

    static vector<ShareDirectoryClient> ResolveDirsGlobbing(const ShareDirectoryClient& dirClient, queue<string> pathSegments, const string& sGlobbingPattern);

    static vector<ShareFileClient> ResolveFilesGlobbing(const ShareDirectoryClient& dirClient, queue<string> pathSegments, const string& sGlobbingPattern);
    
    template<
        typename ClientT,
        vector<ClientT>(*ResolvePathRecursively)(const ShareDirectoryClient&, queue<string>),
        vector<ClientT>(*FindByGlob)(const ShareDirectoryClient&, const string&)
    >
    static vector<ClientT> ResolveGlobbing(const ShareDirectoryClient& dirClient, queue<string> pathSegments, const string& sGlobbingPattern);
    
    static vector<ShareDirectoryClient> ResolveDirsRaw(const ShareDirectoryClient& dirClient, queue<string> pathSegments, const string& sName);
    
    static vector<ShareFileClient> ResolveFilesRaw(const ShareDirectoryClient& dirClient, queue<string> pathSegments, const string& sName);
    
    template<
        typename ClientT,
        vector<ClientT>(*ResolvePathRecursively)(const ShareDirectoryClient&, queue<string>),
        vector<ClientT>(*FindByName)(const ShareDirectoryClient&, const string&)
    >
    static vector<ClientT> ResolveRaw(const ShareDirectoryClient& dirClient, queue<string> pathSegments, const string& sName);
    
    static vector<ShareDirectoryClient> FindDirsByName(const ShareDirectoryClient& dirClient, const string& sName);
    
    static vector<ShareDirectoryClient> FindDirsByGlob(const ShareDirectoryClient& dirClient, const string& sGlob);

    
    static vector<ShareFileClient> FindFilesByName(const ShareDirectoryClient& dirClient, const string& sName);

    static vector<ShareFileClient> FindFilesByGlob(const ShareDirectoryClient& dirClient, const string& sGlob);
    
    static vector<ShareDirectoryClient> FindDirs(const ShareDirectoryClient& dirClient, function<bool(const DirectoryItem&)> Predicate, const string& sPrefix);
    
    static vector<ShareFileClient> FindFiles(const ShareDirectoryClient& dirClient, function<bool(const FileItem&)> Predicate, const string& sPrefix);
    
    template<
        typename ItemT,
        typename ClientT,
        vector<ItemT>(*GetItemsOfPage)(const ListFilesAndDirectoriesPagedResponse&),
        ClientT(*GetClientForItem)(const ShareDirectoryClient&, const ItemT&)
    >
    static vector<ClientT> Find(const ShareDirectoryClient& dirClient, function<bool(const ItemT&)> Predicate, const string& sPrefix);
    
    static vector<DirectoryItem> GetDirsOfPage(const ListFilesAndDirectoriesPagedResponse& pagedResponse);
    
    static vector<FileItem> GetFilesOfPage(const ListFilesAndDirectoriesPagedResponse& pagedResponse);
    
    static ShareDirectoryClient GetDirClient(const ShareDirectoryClient& dirClient, const DirectoryItem& item);
    
    static ShareFileClient GetFileClient(const ShareDirectoryClient& dirClient, const FileItem& item);
    
    template<typename ItemT>
    static bool ItemHasName(const ItemT& item, const string& sName);
    
    template<typename ItemT>
    static bool ItemNameMatches(const ItemT& item, const string& sGlob);
    
    static string PrefixFromName(const string& sName);
    
    static string PrefixFromGlob(const string& sGlob);

    static vector<ShareDirectoryClient> ResolveDirsPathRecursively(const ShareDirectoryClient& dirClient, queue<string> pathSegments)
    {
        try
        {
            if (pathSegments.empty())
            {
                return {};
            }

            const string sUrlPathSegment = pathSegments.front();
            pathSegments.pop();
            if (sUrlPathSegment == "**")
            {
                return ResolveDoubleStar<ShareDirectoryClient, ResolveDirsPathRecursively>(dirClient, pathSegments);
            }

            if (globbing::FindGlobbingChar(sUrlPathSegment) != string::npos)
            {
                return ResolveDirsGlobbing(dirClient, pathSegments, sUrlPathSegment);
            }
            
            return ResolveDirsRaw(dirClient, pathSegments, sUrlPathSegment);
        }
        catch (const Azure::Core::Http::TransportException& exc)
        {
            throw NetworkError();
        }
    }

    static vector<ShareFileClient> ResolveFilesPathRecursively(const ShareDirectoryClient& dirClient, queue<string> pathSegments)
    {
        try
        {
            if (pathSegments.empty())
            {
                return {};
            }

            const string sUrlPathSegment = pathSegments.front();
            pathSegments.pop();
            if (sUrlPathSegment == "**")
            {
                if (pathSegments.empty())
                {
                    return ResolveFilesDoubleStar(dirClient);
                }
                
                return ResolveDoubleStar<ShareFileClient, ResolveFilesPathRecursively>(dirClient, pathSegments);
            }
            
            if (sUrlPathSegment.find_first_of("?[*") != string::npos)
            {
                return ResolveFilesGlobbing(dirClient, pathSegments, sUrlPathSegment);
            }
            
            return ResolveFilesRaw(dirClient, pathSegments, sUrlPathSegment);
        }
        catch (const Azure::Core::Http::TransportException& exc)
        {
            throw NetworkError();
        }
    }

    template<
        typename ClientT,
        vector<ClientT> (*ResolvePathRecursively)(const ShareDirectoryClient&, queue<string>)
    >
    static vector<ClientT> ResolveDoubleStar(const ShareDirectoryClient& dirClient, queue<string> pathSegments)
    {
        vector<ClientT> result = ResolvePathRecursively(dirClient, pathSegments);
        vector<ClientT> subresult;
        for (auto pagedFileAndDirList = dirClient.ListFilesAndDirectories(); pagedFileAndDirList.HasPage(); pagedFileAndDirList.MoveToNextPage())
        {
            for (const auto& dirItem : pagedFileAndDirList.Directories)
            {
                subresult = ResolveDoubleStar<ClientT, ResolvePathRecursively>(dirClient.GetSubdirectoryClient(dirItem.Name), pathSegments);
                result.insert(result.end(), subresult.begin(), subresult.end());
            }
        }
        return result;
    }

    static vector<ShareFileClient> ResolveFilesDoubleStar(const ShareDirectoryClient& dirClient)
    {
        vector<ShareFileClient> result, subresult;
        for (auto pagedFileAndDirList = dirClient.ListFilesAndDirectories(); pagedFileAndDirList.HasPage(); pagedFileAndDirList.MoveToNextPage())
        {
            for (const auto& fileItem : pagedFileAndDirList.Files)
            {
                result.push_back(dirClient.GetFileClient(fileItem.Name));
            }
            for (const auto& dirItem : pagedFileAndDirList.Directories)
            {
                subresult = ResolveFilesDoubleStar(dirClient.GetSubdirectoryClient(dirItem.Name));
                result.insert(result.end(), subresult.begin(), subresult.end());
            }
        }
        return result;
    }

    static vector<ShareDirectoryClient> ResolveDirsGlobbing(const ShareDirectoryClient& dirClient, queue<string> pathSegments, const string& sGlobbingPattern)
    {
        return ResolveGlobbing<ShareDirectoryClient, ResolveDirsPathRecursively, FindDirsByGlob>(dirClient, pathSegments, sGlobbingPattern);
    }

    static vector<ShareFileClient> ResolveFilesGlobbing(const ShareDirectoryClient& dirClient, queue<string> pathSegments, const string& sGlobbingPattern)
    {
        return ResolveGlobbing<ShareFileClient, ResolveFilesPathRecursively, FindFilesByGlob>(dirClient, pathSegments, sGlobbingPattern);
    }

    template<
        typename ClientT,
        vector<ClientT>(*ResolvePathRecursively)(const ShareDirectoryClient&, queue<string>),
        vector<ClientT> (*FindByGlob)(const ShareDirectoryClient&, const string&)
    >
    static vector<ClientT> ResolveGlobbing(const ShareDirectoryClient& dirClient, queue<string> pathSegments, const string& sGlobbingPattern)
    {
        if (pathSegments.empty())
        {
            return FindByGlob(dirClient, sGlobbingPattern);
        }

        vector<ClientT> result, subresult;
        for (const ShareDirectoryClient& subdirClient : FindDirsByGlob(dirClient, sGlobbingPattern))
        {
            subresult = ResolvePathRecursively(subdirClient, pathSegments);
            result.insert(result.end(), subresult.begin(), subresult.end());
        }
        return result;
    }

    static vector<ShareDirectoryClient> ResolveDirsRaw(const ShareDirectoryClient& dirClient, queue<string> pathSegments, const string& sName)
    {
        return ResolveRaw<ShareDirectoryClient, ResolveDirsPathRecursively, FindDirsByName>(dirClient, pathSegments, sName);
    }

    static vector<ShareFileClient> ResolveFilesRaw(const ShareDirectoryClient& dirClient, queue<string> pathSegments, const string& sName)
    {
        return ResolveRaw<ShareFileClient, ResolveFilesPathRecursively, FindFilesByName>(dirClient, pathSegments, sName);
    }

    template<
        typename ClientT,
        vector<ClientT>(*ResolvePathRecursively)(const ShareDirectoryClient&, queue<string>),
        vector<ClientT> (*FindByName)(const ShareDirectoryClient&, const string&)
    >
    static vector<ClientT> ResolveRaw(const ShareDirectoryClient& dirClient, queue<string> pathSegments, const string& sName)
    {
        if (pathSegments.empty())
        {
            return FindByName(dirClient, sName);
        }

        vector<ShareDirectoryClient> foundDirs = FindDirsByName(dirClient, sName);
        return foundDirs.empty() ? vector<ClientT>() : ResolvePathRecursively(foundDirs.front(), pathSegments);
    }

    static vector<ShareDirectoryClient> FindDirsByName(const ShareDirectoryClient& dirClient, const string& sName)
    {
        return FindDirs(dirClient, [sName](const DirectoryItem& item) { return ItemHasName(item, sName); }, PrefixFromName(sName));
    }

    static vector<ShareDirectoryClient> FindDirsByGlob(const ShareDirectoryClient& dirClient, const string& sGlob)
    {
        return FindDirs(dirClient, [sGlob](const DirectoryItem& item) { return ItemNameMatches(item, sGlob); }, PrefixFromGlob(sGlob));
    }

    static vector<ShareFileClient> FindFilesByName(const ShareDirectoryClient& dirClient, const string& sName)
    {
        return FindFiles(dirClient, [sName](const FileItem& item) { return ItemHasName(item, sName); }, PrefixFromName(sName));
    }

    static vector<ShareFileClient> FindFilesByGlob(const ShareDirectoryClient& dirClient, const string& sGlob)
    {
        return FindFiles(dirClient, [sGlob](const FileItem& item) { return ItemNameMatches(item, sGlob); }, PrefixFromGlob(sGlob));
    }

    static vector<ShareDirectoryClient> FindDirs(const ShareDirectoryClient& dirClient, function<bool(const DirectoryItem&)> Predicate, const string& sPrefix)
    {
        return Find<DirectoryItem, ShareDirectoryClient, GetDirsOfPage, GetDirClient>(dirClient, Predicate, sPrefix);
    }

    static vector<ShareFileClient> FindFiles(const ShareDirectoryClient& dirClient, function<bool(const FileItem&)> Predicate, const string& sPrefix)
    {
        return Find<FileItem, ShareFileClient, GetFilesOfPage, GetFileClient>(dirClient, Predicate, sPrefix);
    }

    template<
        typename ItemT,
        typename ClientT,
        vector<ItemT> (*GetItemsOfPage)(const ListFilesAndDirectoriesPagedResponse&),
        ClientT (*GetClientForItem)(const ShareDirectoryClient&, const ItemT&)
    >
    static vector<ClientT> Find(const ShareDirectoryClient& dirClient, function<bool(const ItemT&)> Predicate, const string& sPrefix)
    {
        ListFilesAndDirectoriesOptions opts;
        opts.Prefix = sPrefix;

        vector<ClientT> result;
        for (auto pagedFileAndDirList = dirClient.ListFilesAndDirectories(opts); pagedFileAndDirList.HasPage(); pagedFileAndDirList.MoveToNextPage())
        {
            for (const auto& item : GetItemsOfPage(pagedFileAndDirList))
            {
                if (Predicate(item))
                {
                    result.push_back(GetClientForItem(dirClient, item));
                }
            }
        }
        return result;
    }

    static vector<DirectoryItem> GetDirsOfPage(const ListFilesAndDirectoriesPagedResponse& pagedResponse)
    {
        return pagedResponse.Directories;
    }

    static vector<FileItem> GetFilesOfPage(const ListFilesAndDirectoriesPagedResponse& pagedResponse)
    {
        return pagedResponse.Files;
    }

    static ShareDirectoryClient GetDirClient(const ShareDirectoryClient& dirClient, const DirectoryItem& item)
    {
        return dirClient.GetSubdirectoryClient(item.Name);
    }

    static ShareFileClient GetFileClient(const ShareDirectoryClient& dirClient, const FileItem& item)
    {
        return dirClient.GetFileClient(item.Name);
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
}
