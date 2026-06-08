#include "khiops_driver_azure/util.hpp"
#include <sstream>
#include <queue>
#include <deque>
#include <azure/identity.hpp>
#include "khiops_driver_common/logging.hpp"
#include "khiops_driver_common/util.hpp"
#include "khiops_driver_azure/connstr.hpp"
#include "khiops_driver_azure/blobpathresolve.hpp"
#include "khiops_driver_azure/sharepathresolve.hpp"

using namespace std;
using namespace khiops_driver_common;

namespace khiops_driver_azure {

int StorageTypeOfUrl(StorageType *result, const Azure::Core::Url &url, bool is_emulated_storage) {
    std::string host = url.GetHost();
    if (is_emulated_storage) {
        // The emulator supports only blob storage services, not file share storage services.
        *result = BLOB;
    } else if (EndsWith(host, ".blob.core.windows.net")) {
        *result = BLOB;
    } else if (EndsWith(host, ".file.core.windows.net")) {
        *result = FILE_SHARE;
    } else {
        GetLogger()->error("URL {} contains invalid domain.", url.GetAbsoluteUrl());
        return -1;
    }
    return 0;
}

string ObjectPathToString(const ObjectPath &object_path) {
    ostringstream oss;
    oss << "ObjectPath("
        << "emulated_account_name=" << (object_path.emulated_account_name == nullptr ? "<none>" : *object_path.emulated_account_name) << ", "
        << "blob_container=" << (object_path.blob_container == nullptr ? "<none>" : *object_path.blob_container) << ", "
        << "blob=" << (object_path.blob == nullptr ? "<none>" : *object_path.blob) << ", "
        << "file_share=" << (object_path.file_share == nullptr ? "<none>" : *object_path.file_share) << ", "
        << "file_path=";
    if (object_path.file_path == nullptr) {
        oss << "<none>";
    } else {
        oss << "[";
        for (size_t i = 0ULL; i < object_path.file_path->size(); i++) {
            if (i > 0ULL) {
                oss << ", ";
            }
            oss << (*object_path.file_path)[i];
        }
        oss << "]";
    }
    oss << ")";
    return oss.str();
}

int ObjectPathOfUrl(ObjectPath *result, const Azure::Core::Url &url, bool is_emulated_storage, StorageType storage_type) {
    string path = url.GetPath();
    smatch match;
    if (is_emulated_storage) {  // Emulated BLOB storage
        if (regex_match(path, match, regex("([^/]+)/([^/]+)/(.+)"))) {
            //  accountname/container/object
            // OR
            //  accountname/container/object/
            *result = ObjectPath();
            result->emulated_account_name = make_unique<string>(match[1].str());
            result->blob_container = make_unique<string>(match[2].str());
            result->blob = make_unique<string>(match[3].str());
            return 0;
        } else {
            GetLogger()->error("Invalid emulated storage object path: {}.", path);
        }
    } else if (storage_type == BLOB) {  // Real Azure cloud BLOB storage
        if (regex_match(path, match, regex("([^/]+)/(.+)"))) {
            //  container/object
            // OR
            //  container/object/
            *result = ObjectPath();
            result->blob_container = make_unique<string>(match[1].str());
            result->blob = make_unique<string>(match[2].str());
            return 0;
        } else {
            GetLogger()->error("Invalid cloud blob path: {}.", path);
        }
    } else {  // Real Azure cloud SHARE storage
        if (regex_match(path, match, regex("([^/]+)((?:/[^/]+)+/?)"))) {
            //  share/path/to/a/file
            // OR
            //  share/path/to/a/dir/
            *result = ObjectPath();
            result->file_share = make_unique<string>(match[1].str());
            result->file_path = make_unique<vector<string>>(Split(match[2].str(), '/', -1, true));
            return 0;
        } else {
            GetLogger()->error("Invalid cloud file path: {}.", path);
        }
    }
    return -1;
}

bool IsEmulatedStorage() {
    string sEmulatedStorageEnvVarVal = GetEnvVar("AZURE_EMULATED_STORAGE");
    return !sEmulatedStorageEnvVarVal.empty() && sEmulatedStorageEnvVarVal != "false";
}

const RemoteObjectRequestUserData *GetUserData(const RemoteObjectRequest &request) {
    return static_cast<const RemoteObjectRequestUserData *>(request.user_data.get());
}

string GetBlobContainerUrl(const RemoteObjectRequest &request) {
    const RemoteObjectRequestUserData* request_user_data = GetUserData(request);
    ostringstream oss;
    oss << request_user_data->service_url << "/" << *request_user_data->object_path.blob_container;
    std::string result = oss.str();
    GetLogger()->debug("Blob container URL is: {}.", result);
    return result;
}

Azure::Storage::Blobs::BlobContainerClient GetBlobContainerClient(const RemoteObjectRequest &request) {
    const RemoteObjectRequestUserData* request_user_data = GetUserData(request);
    if (request_user_data->is_using_connection_string) {
        return Azure::Storage::Blobs::BlobContainerClient(GetBlobContainerUrl(request), request_user_data->connection_string_credential);
    } else {
        return Azure::Storage::Blobs::BlobContainerClient(GetBlobContainerUrl(request), request_user_data->no_connection_string_credential);
    }
}

vector<string> ListBlobs(const RemoteObjectRequest &request) {
    return ResolveBlobsSearchString(GetBlobContainerClient(request), *GetUserData(request)->object_path.blob);
}

int GetBlobClient(Azure::Storage::Blobs::BlobClient *result, const RemoteObjectRequest &request, const string &url) {
    if (CheckIsNotGlobbingPattern(url) == 0) {
        const RemoteObjectRequestUserData* request_user_data = GetUserData(request);
        if (request_user_data->is_using_connection_string) {
            *result = std::move(Azure::Storage::Blobs::BlobClient(url, request_user_data->connection_string_credential));
        } else {
            *result = std::move(Azure::Storage::Blobs::BlobClient(url, request_user_data->no_connection_string_credential));
        }
        return 0;
    }
    return -1;
}

int GetBlobClient(Azure::Storage::Blobs::BlobClient *result, const RemoteObjectRequest &request) {
    return GetBlobClient(result, request, request.url);
}

string GetFileShareUrl(const RemoteObjectRequest &request) {
    const RemoteObjectRequestUserData* request_user_data = GetUserData(request);
    ostringstream oss;
    oss << request_user_data->service_url << "/" << *request_user_data->object_path.file_share;
    string result = oss.str();
    GetLogger()->debug("File share URL is: {}.", result);
    return result;
}

Azure::Storage::Files::Shares::ShareClient
GetShareClient(const RemoteObjectRequest &request) {
    const RemoteObjectRequestUserData* request_user_data = GetUserData(request);
    Azure::Storage::Files::Shares::ShareClientOptions opts;
    opts.ShareTokenIntent =
            Azure::Storage::Files::Shares::Models::ShareTokenIntent::Backup;
    if (request_user_data->is_using_connection_string) {
        return Azure::Storage::Files::Shares::ShareClient(
                GetFileShareUrl(request), request_user_data->connection_string_credential, opts);
    } else {
        return Azure::Storage::Files::Shares::ShareClient(
                GetFileShareUrl(request), request_user_data->no_connection_string_credential, opts);
    }
}

Azure::Storage::Files::Shares::ShareDirectoryClient
GetDirClient(const RemoteObjectRequest &request, const string &url) {
    const RemoteObjectRequestUserData* request_user_data = GetUserData(request);
    Azure::Storage::Files::Shares::ShareClientOptions opts;
    opts.ShareTokenIntent =
            Azure::Storage::Files::Shares::Models::ShareTokenIntent::Backup;
    if (request_user_data->is_using_connection_string) {
        return Azure::Storage::Files::Shares::ShareDirectoryClient(
                url, request_user_data->connection_string_credential, opts);
    } else {
        return Azure::Storage::Files::Shares::ShareDirectoryClient(
                url, request_user_data->no_connection_string_credential, opts);
    }
}

Azure::Storage::Files::Shares::ShareDirectoryClient
GetDirClient(const RemoteObjectRequest &request) {
    return GetShareClient(request).GetRootDirectoryClient();
}

vector<string>
ListDirs(const RemoteObjectRequest &request) {
    const RemoteObjectRequestUserData *user_data = GetUserData(request);
    return ResolveDirsPathRecursively(
            GetDirClient(request),
            queue<string, deque<string>>(
                  deque<string>(user_data->object_path.file_path->begin(), user_data->object_path.file_path->end())));
}

vector<string>
ListFiles(const RemoteObjectRequest &request) {
    const RemoteObjectRequestUserData *user_data = GetUserData(request);
    return ResolveFilesPathRecursively(
            GetDirClient(request),
            queue<string, deque<string>>(
                  deque<string>(user_data->object_path.file_path->begin(), user_data->object_path.file_path->end())));

}

int GetParentDir(
        Azure::Storage::Files::Shares::ShareDirectoryClient *result,
        const RemoteObjectRequest &request) {
    Azure::Storage::Files::Shares::ShareDirectoryClient dirClient =
            GetDirClient(request);
    vector<string> path = *GetUserData(request)->object_path.file_path;
    path.pop_back();

    for (string sPathFragment : path) {
        Azure::Storage::Files::Shares::ListFilesAndDirectoriesOptions opts;
        opts.Prefix = sPathFragment;

        bool bAlreadyExisting = false;
        for (auto pagedResponse = dirClient.ListFilesAndDirectories(opts);
                 pagedResponse.HasPage(); pagedResponse.MoveToNextPage()) {
            if (find_if(pagedResponse.Directories.begin(),
                          pagedResponse.Directories.end(),
                          [sPathFragment](const auto &dirItem) {
                            return dirItem.Name == sPathFragment;
                          }) != pagedResponse.Directories.end()) {
                bAlreadyExisting = true;
                break;
            }
        }

        if (!bAlreadyExisting) {
            GetLogger()->error("Ancestor directory {}/{} does not exist.",
                                 dirClient.GetUrl(), sPathFragment,
                                 request.url);
            return -1;
        }

        dirClient = dirClient.GetSubdirectoryClient(sPathFragment);
    }

    if (result)
        *result = dirClient;
    return 0;
}

int GetFileClient(Azure::Storage::Files::Shares::ShareFileClient *result, const RemoteObjectRequest &request, const std::string &url) {
    if (CheckIsNotGlobbingPattern(url) == 0) {
        const RemoteObjectRequestUserData* request_user_data = GetUserData(request);
        Azure::Storage::Files::Shares::ShareClientOptions opts;
        opts.ShareTokenIntent =
                Azure::Storage::Files::Shares::Models::ShareTokenIntent::Backup;
        if (request_user_data->is_using_connection_string) {
            *result = std::move(Azure::Storage::Files::Shares::ShareFileClient(url, request_user_data->connection_string_credential, opts));
        } else {
            *result = std::move(Azure::Storage::Files::Shares::ShareFileClient(url, request_user_data->no_connection_string_credential, opts));
        }
        return 0;
    }
    return -1;
}

int GetFileClient(Azure::Storage::Files::Shares::ShareFileClient *result, const RemoteObjectRequest &request) {
    return GetFileClient(result, request, request.url);
}

vector<string> ListBlobsOrFiles(const RemoteObjectRequest &request) {
    return GetUserData(request)->storage_type == BLOB ? ListBlobs(request) : ListFiles(request);
}

int ListBlobsOrFilesCheckNotEmpty(vector<string> *result, const RemoteObjectRequest &request) {
    vector<string> objects = ListBlobsOrFiles(request);
    if (objects.empty()) {
        GetLogger()->error("No object matches URL {}.", request.url);
        return -1;
    } else {
        *result = std::move(objects);
        return 0;
    }
}

}