#include "urlresolve.hpp"
#include <functional>
#include "glob.hpp"
#include "../exception.hpp"
#include "../contrib/globmatch.hpp"

namespace az
{
    using ShareDirectoryClient = Azure::Storage::Files::Shares::ShareDirectoryClient;
    using ShareFileClient = Azure::Storage::Files::Shares::ShareFileClient;
    using DirectoryItem = Azure::Storage::Files::Shares::Models::DirectoryItem;
    using FileItem = Azure::Storage::Files::Shares::Models::FileItem;
    using ListFilesAndDirectoriesPagedResponse = Azure::Storage::Files::Shares::ListFilesAndDirectoriesPagedResponse;
    using ListFilesAndDirectoriesOptions = Azure::Storage::Files::Shares::ListFilesAndDirectoriesOptions;

    vector<ShareDirectoryClient> ResolveDirsUrlRecursively(const ShareDirectoryClient& dirClient, queue<string> urlPathSegments);

    vector<ShareFileClient> ResolveFilesUrlRecursively(const ShareDirectoryClient& dirClient, queue<string> urlPathSegments);

    template<
        typename ClientT,
        vector<ClientT>(*ResolveUrlRecursively)(const ShareDirectoryClient&, queue<string>)
    >
    static vector<ClientT> ResolveDoubleStar(const ShareDirectoryClient& dirClient, queue<string> urlPathSegments);

    static vector<ShareFileClient> ResolveFilesDoubleStar(const ShareDirectoryClient& dirClient);

    static vector<ShareDirectoryClient> ResolveDirsGlobbing(const ShareDirectoryClient& dirClient, queue<string> urlPathSegments, const string& sGlobbingPattern);

    static vector<ShareFileClient> ResolveFilesGlobbing(const ShareDirectoryClient& dirClient, queue<string> urlPathSegments, const string& sGlobbingPattern);
    
    template<
        typename ClientT,
        vector<ClientT>(*ResolveUrlRecursively)(const ShareDirectoryClient&, queue<string>),
        vector<ClientT>(*FindByGlob)(const ShareDirectoryClient&, const string&)
    >
    static vector<ClientT> ResolveGlobbing(const ShareDirectoryClient& dirClient, queue<string> urlPathSegments, const string& sGlobbingPattern);
    
    static vector<ShareDirectoryClient> ResolveDirsRaw(const ShareDirectoryClient& dirClient, queue<string> urlPathSegments, const string& sName);
    
    static vector<ShareFileClient> ResolveFilesRaw(const ShareDirectoryClient& dirClient, queue<string> urlPathSegments, const string& sName);
    
    template<
        typename ClientT,
        vector<ClientT>(*ResolveUrlRecursively)(const ShareDirectoryClient&, queue<string>),
        vector<ClientT>(*FindByName)(const ShareDirectoryClient&, const string&)
    >
    static vector<ClientT> ResolveRaw(const ShareDirectoryClient& dirClient, queue<string> urlPathSegments, const string& sName);
    
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

    static vector<ShareDirectoryClient> ResolveDirsUrlRecursively(const ShareDirectoryClient& dirClient, queue<string> urlPathSegments)
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
                return ResolveDoubleStar<ShareDirectoryClient, ResolveDirsUrlRecursively>(dirClient, urlPathSegments);
            }

            if (sUrlPathSegment.find_first_of("?[*") != string::npos)
            {
                return ResolveDirsGlobbing(dirClient, urlPathSegments, sUrlPathSegment);
            }
            
            return ResolveDirsRaw(dirClient, urlPathSegments, sUrlPathSegment);
        }
        catch (const Azure::Core::Http::TransportException& exc)
        {
            throw NetworkError();
        }
    }

    static vector<ShareFileClient> ResolveFilesUrlRecursively(const ShareDirectoryClient& dirClient, queue<string> urlPathSegments)
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
                if (urlPathSegments.empty())
                {
                    return ResolveFilesDoubleStar(dirClient);
                }
                
                return ResolveDoubleStar<ShareFileClient, ResolveFilesUrlRecursively>(dirClient, urlPathSegments);
            }
            
            if (sUrlPathSegment.find_first_of("?[*") != string::npos)
            {
                return ResolveFilesGlobbing(dirClient, urlPathSegments, sUrlPathSegment);
            }
            
            return ResolveFilesRaw(dirClient, urlPathSegments, sUrlPathSegment);
        }
        catch (const Azure::Core::Http::TransportException& exc)
        {
            throw NetworkError();
        }
    }

    template<
        typename ClientT,
        vector<ClientT> (*ResolveUrlRecursively)(const ShareDirectoryClient&, queue<string>)
    >
    static vector<ClientT> ResolveDoubleStar(const ShareDirectoryClient& dirClient, queue<string> urlPathSegments)
    {
        vector<ClientT> result = ResolveUrlRecursively(dirClient, urlPathSegments);
        vector<ClientT> subresult;
        for (auto pagedFileAndDirList = dirClient.ListFilesAndDirectories(); pagedFileAndDirList.HasPage(); pagedFileAndDirList.MoveToNextPage())
        {
            for (const auto& dirItem : pagedFileAndDirList.Directories)
            {
                subresult = ResolveDoubleStar<ClientT, ResolveUrlRecursively>(dirClient.GetSubdirectoryClient(dirItem.Name), urlPathSegments);
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

    static vector<ShareDirectoryClient> ResolveDirsGlobbing(const ShareDirectoryClient& dirClient, queue<string> urlPathSegments, const string& sGlobbingPattern)
    {
        return ResolveGlobbing<ShareDirectoryClient, ResolveDirsUrlRecursively, FindDirsByGlob>(dirClient, urlPathSegments, sGlobbingPattern);
    }

    static vector<ShareFileClient> ResolveFilesGlobbing(const ShareDirectoryClient& dirClient, queue<string> urlPathSegments, const string& sGlobbingPattern)
    {
        return ResolveGlobbing<ShareFileClient, ResolveFilesUrlRecursively, FindFilesByGlob>(dirClient, urlPathSegments, sGlobbingPattern);
    }

    template<
        typename ClientT,
        vector<ClientT>(*ResolveUrlRecursively)(const ShareDirectoryClient&, queue<string>),
        vector<ClientT> (*FindByGlob)(const ShareDirectoryClient&, const string&)
    >
    static vector<ClientT> ResolveGlobbing(const ShareDirectoryClient& dirClient, queue<string> urlPathSegments, const string& sGlobbingPattern)
    {
        if (urlPathSegments.empty())
        {
            return FindByGlob(dirClient, sGlobbingPattern);
        }

        vector<ClientT> result, subresult;
        for (const ShareDirectoryClient& subdirClient : FindDirsByGlob(dirClient, sGlobbingPattern))
        {
            subresult = ResolveUrlRecursively(subdirClient, urlPathSegments);
            result.insert(result.end(), subresult.begin(), subresult.end());
        }
        return result;
    }

    static vector<ShareDirectoryClient> ResolveDirsRaw(const ShareDirectoryClient& dirClient, queue<string> urlPathSegments, const string& sName)
    {
        return ResolveRaw<ShareDirectoryClient, ResolveDirsUrlRecursively, FindDirsByName>(dirClient, urlPathSegments, sName);
    }

    static vector<ShareFileClient> ResolveFilesRaw(const ShareDirectoryClient& dirClient, queue<string> urlPathSegments, const string& sName)
    {
        return ResolveRaw<ShareFileClient, ResolveFilesUrlRecursively, FindFilesByName>(dirClient, urlPathSegments, sName);
    }

    template<
        typename ClientT,
        vector<ClientT>(*ResolveUrlRecursively)(const ShareDirectoryClient&, queue<string>),
        vector<ClientT> (*FindByName)(const ShareDirectoryClient&, const string&)
    >
    static vector<ClientT> ResolveRaw(const ShareDirectoryClient& dirClient, queue<string> urlPathSegments, const string& sName)
    {
        if (urlPathSegments.empty())
        {
            return FindByName(dirClient, sName);
        }

        vector<ShareDirectoryClient> foundDirs = FindDirsByName(dirClient, sName);
        return foundDirs.empty() ? vector<ClientT>() : ResolveUrlRecursively(foundDirs.front(), urlPathSegments);
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
