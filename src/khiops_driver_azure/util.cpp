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
    } else if (util::str::EndsWith(host, ".blob.core.windows.net")) {
        *result = BLOB;
    } else if (util::str::EndsWith(host, ".file.core.windows.net")) {
        *result = SHARE;
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
            result->file_path = make_unique<vector<string>>(util::str::Split(match[2].str(), '/', -1, true));
            return 0;
        } else {
            GetLogger()->error("Invalid cloud file path: {}.", path);
        }
    }
    return -1;
}

bool IsEmulatedStorage() {
    string sEmulatedStorageEnvVarVal = util::env::GetEnvVar("AZURE_EMULATED_STORAGE");
    return !sEmulatedStorageEnvVarVal.empty() && sEmulatedStorageEnvVarVal != "false";
}

int BuildServiceRequest(unique_ptr<ServiceRequest> *result, const string &url) {
    unique_ptr<ServiceRequest> request = make_unique<ServiceRequest>();

    // Perform initial URL parsing using Azure SDK.
    try {
        request->azure_url = Azure::Core::Url(url);
    } catch (const exception &) {
        GetLogger()->error("Caught an exception while performing basic URL parsing: URL {} is invalid.", url);
        return -1;
    }

    // Determine if the requested object is a file or a directory.
    request->is_dir = util::IsDirUrl(url);

    // Determine if storage service is emulated or not.
    request->is_emulated_storage = IsEmulatedStorage();

    // Get type of storage service.
    if (StorageTypeOfUrl(&request->storage_type, request->azure_url, request->is_emulated_storage) == 0) {
        if (ObjectPathOfUrl(&request->object_path, request->azure_url, request->is_emulated_storage, request->storage_type) == 0) {
            // Parse connection string.
            string connection_string_as_string = util::env::GetEnvVar("AZURE_STORAGE_CONNECTION_STRING");
            request->is_using_connection_string = !connection_string_as_string.empty();
            if (request->is_emulated_storage && !request->is_using_connection_string) {
                GetLogger()->error("Undefined or empty environment variable: AZURE_STORAGE_CONNECTION_STRING.");
                return -1;
            }
            connstr::ConnectionString connection_string;
            if (connstr::ConnectionString::ParseConnectionString(&connection_string, connection_string_as_string, request->is_emulated_storage) == 0) {
                if (connection_string.CheckAgainstUrl(request->azure_url, request->storage_type) == 0) {
                    // Credentials
                    if (request->is_using_connection_string) {
                        request->connection_string_credential = make_shared<Azure::Storage::StorageSharedKeyCredential>(connection_string.sAccountName, connection_string.sAccountKey);
                    } else {
                        request->no_connection_string_credential = make_shared<Azure::Identity::ChainedTokenCredential>(
                            Azure::Identity::ChainedTokenCredential::Sources {
                                std::make_shared<Azure::Identity::EnvironmentCredential>(),  // for Client ID + Client Secret or Certificate environment variables
                                std::make_shared<Azure::Identity::WorkloadIdentityCredential>(),
                                std::make_shared<Azure::Identity::ManagedIdentityCredential>(),
                                std::make_shared<Azure::Identity::AzureCliCredential>()
                            }
                        );
                    }
                    GetLogger()->debug("Just built the following service request:");
                    LogServiceRequest(*request);
                    *result = std::move(request);
                }
            }
        }
    }

    return -1;
}

void LogServiceRequest(const ServiceRequest &request) {
    khiops_driver_common::GetLogger()->debug("Service request details:");
    khiops_driver_common::GetLogger()->debug("  URL: {}", request.azure_url.GetAbsoluteUrl());
    khiops_driver_common::GetLogger()->debug("  object type: {}", request.is_dir ? "directory" : "file (in the sense of not being a directory)");
    khiops_driver_common::GetLogger()->debug("  is storage emulated? {}", request.is_emulated_storage ? "yes" : "no");
    khiops_driver_common::GetLogger()->debug("  storage type: {}", request.storage_type == BLOB ? "blob" : "file share");
    khiops_driver_common::GetLogger()->debug("  object path: {}", ObjectPathToString(request.object_path));
    khiops_driver_common::GetLogger()->debug("  is using connection string? {}", request.is_using_connection_string ? "yes" : "no");
}

string GetServiceUrl(const ServiceRequest &request) {
    ostringstream oss;
    if (request.is_emulated_storage) {  // Emulated BLOB storage
        oss << request.azure_url.GetScheme() << "://" << request.azure_url.GetHost()
            << ":" << request.azure_url.GetPort() << "/"
            << *request.object_path.emulated_account_name;
    } else if (request.storage_type == BLOB) {  // Real Azure cloud BLOB storage
        oss << request.azure_url.GetScheme() << "://" << request.azure_url.GetHost()
            << ":" << request.azure_url.GetPort();
    } else {  // Real Azure cloud FILE storage
        oss << request.azure_url.GetScheme() << "://" << request.azure_url.GetHost()
            << ":" << request.azure_url.GetPort();
    }
    string result = oss.str();
    GetLogger()->debug("Service URL is: {}.", result);
    return result;
}

string GetBlobContainerUrl(const ServiceRequest &request) {
    ostringstream oss;
    oss << GetServiceUrl(request) << "/" << *request.object_path.blob_container;
    std::string result = oss.str();
    GetLogger()->debug("Blob container URL is: {}.", result);
    return result;
}

Azure::Storage::Blobs::BlobContainerClient GetBlobContainerClient(const ServiceRequest &request) {
    if (request.is_using_connection_string) {
        return Azure::Storage::Blobs::BlobContainerClient(GetBlobContainerUrl(request), request.connection_string_credential);
    } else {
        return Azure::Storage::Blobs::BlobContainerClient(GetBlobContainerUrl(request), request.no_connection_string_credential);
    }
}

vector<string> ListBlobs(const ServiceRequest &request) {
    return ResolveBlobsSearchString(GetBlobContainerClient(request), *request.object_path.blob);
}

int GetBlobClient(Azure::Storage::Blobs::BlobClient *result, const ServiceRequest &request, const string &url) {
    if (util::glob::CheckIsNotGlobbingPattern(url) == 0) {
        if (request.is_using_connection_string) {
            *result = std::move(Azure::Storage::Blobs::BlobClient(url, request.connection_string_credential));
        } else {
            *result = std::move(Azure::Storage::Blobs::BlobClient(url, request.no_connection_string_credential));
        }
        return 0;
    }
    return -1;
}

int GetBlobClient(Azure::Storage::Blobs::BlobClient *result, const ServiceRequest &request) {
    return GetBlobClient(result, request, request.azure_url.GetAbsoluteUrl());
}

string GetFileShareUrl(const ServiceRequest &request) {
    ostringstream oss;
    oss << GetServiceUrl(request) << "/" << *request.object_path.file_share;
    string result = oss.str();
    GetLogger()->debug("File share URL is: {}.", result);
    return result;
}

Azure::Storage::Files::Shares::ShareClient
GetShareClient(const ServiceRequest &request) {
    Azure::Storage::Files::Shares::ShareClientOptions opts;
    opts.ShareTokenIntent =
            Azure::Storage::Files::Shares::Models::ShareTokenIntent::Backup;
    if (request.is_using_connection_string) {
        return Azure::Storage::Files::Shares::ShareClient(
                GetFileShareUrl(request), request.connection_string_credential, opts);
    } else {
        return Azure::Storage::Files::Shares::ShareClient(
                GetFileShareUrl(request), request.no_connection_string_credential, opts);
    }
}

Azure::Storage::Files::Shares::ShareDirectoryClient
GetDirClient(const ServiceRequest &request, const string &url) {
    Azure::Storage::Files::Shares::ShareClientOptions opts;
    opts.ShareTokenIntent =
            Azure::Storage::Files::Shares::Models::ShareTokenIntent::Backup;
    if (request.is_using_connection_string) {
        return Azure::Storage::Files::Shares::ShareDirectoryClient(
                url, request.connection_string_credential, opts);
    } else {
        return Azure::Storage::Files::Shares::ShareDirectoryClient(
                url, request.no_connection_string_credential, opts);
    }
}

Azure::Storage::Files::Shares::ShareDirectoryClient
GetDirClient(const ServiceRequest &request) {
    return GetShareClient(request).GetRootDirectoryClient();
}

vector<string>
ListDirs(const ServiceRequest &request) {
    return ResolveDirsPathRecursively(
            GetDirClient(request),
            queue<string, deque<string>>(
                  deque<string>(request.object_path.file_path->begin(), request.object_path.file_path->end())));
}

vector<string>
ListFiles(const ServiceRequest &request) {
    return ResolveFilesPathRecursively(
            GetDirClient(request),
            queue<string, deque<string>>(
                  deque<string>(request.object_path.file_path->begin(), request.object_path.file_path->end())));

}

int GetParentDir(
        Azure::Storage::Files::Shares::ShareDirectoryClient *result,
        const ServiceRequest &request) {
    Azure::Storage::Files::Shares::ShareDirectoryClient dirClient =
            GetDirClient(request);
    vector<string> path = *request.object_path.file_path;
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
                                 request.azure_url.GetAbsoluteUrl());
            return -1;
        }

        dirClient = dirClient.GetSubdirectoryClient(sPathFragment);
    }

    if (result)
        *result = dirClient;
    return 0;
}

int GetFileClient(Azure::Storage::Files::Shares::ShareFileClient *result, const ServiceRequest &request, const std::string &url) {
    if (util::glob::CheckIsNotGlobbingPattern(url) == 0) {
        Azure::Storage::Files::Shares::ShareClientOptions opts;
        opts.ShareTokenIntent =
                Azure::Storage::Files::Shares::Models::ShareTokenIntent::Backup;
        if (request.is_using_connection_string) {
            *result = std::move(Azure::Storage::Files::Shares::ShareFileClient(url, request.connection_string_credential, opts));
        } else {
            *result = std::move(Azure::Storage::Files::Shares::ShareFileClient(url, request.no_connection_string_credential, opts));
        }
        return 0;
    }
    return -1;
}

int GetFileClient(Azure::Storage::Files::Shares::ShareFileClient *result, const ServiceRequest &request) {
    return GetFileClient(result, request, request.azure_url.GetAbsoluteUrl());
}

vector<string> ListBlobsOrFiles(const ServiceRequest &request) {
    return request.storage_type == BLOB ? ListBlobs(request) : ListFiles(request);
}

int ListBlobsOrFilesCheckNotEmpty(vector<string> *result, const ServiceRequest &request) {
    vector<string> objects = ListBlobsOrFiles(request);
    if (objects.empty()) {
        GetLogger()->error("No object matches URL {}.", request.azure_url.GetAbsoluteUrl());
        return -1;
    } else {
        *result = std::move(objects);
        return 0;
    }
}

}