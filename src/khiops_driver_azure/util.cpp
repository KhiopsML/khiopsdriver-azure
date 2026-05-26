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
        if (regex_match(sPath, match, regex("([^/]+)/(.+)"))) {
            //  container/object
            // OR
            //  container/object/
            *result = ObjectPath();
            result->blob_container = make_unique<string>(match[1].str());
            result->blob = make_unique<string>(match[2].str());
            return 0;
        } else {
            GetLogger()->error("Invalid cloud blob path: {}.", sPath);
        }
    } else {  // Real Azure cloud SHARE storage
        if (regex_match(sPath, match, regex("([^/]+)((?:/[^/]+)+/?)"))) {
            //  share/path/to/a/file
            // OR
            //  share/path/to/a/dir/
            *result = ObjectPath();
            result->file_share = make_unique<string>(match[1].str());
            result->file_path = make_unique<vector<string>>(util::str::Split(match[2].str(), '/', -1, true));
            return 0;
        } else {
            GetLogger()->error("Invalid cloud file path: {}.", sPath);
        }
    }
    return -1;
}

bool IsEmulatedStorage() {
    string sEmulatedStorageEnvVarVal = util::env::GetEnvVar("AZURE_EMULATED_STORAGE");
    return !sEmulatedStorageEnvVarVal.empty() && sEmulatedStorageEnvVarVal != "false";
}

int BuildServiceRequest(ServiceRequest *result, const string &url) {
    ServiceRequest request;

    // Perform initial URL parsing using Azure SDK.
    try {
        request.azure_url = Azure::Core::Url(url);
    } catch (const exception &) {
        GetLogger()->error("Caught an exception while performing basic URL parsing: URL {} is invalid.", url);
        return -1;
    }

    // Determine if the requested object is a file or a directory.
    request.is_dir = util::IsDirUrl(url);

    // Determine if storage service is emulated or not.
    request.is_emulated_storage = IsEmulatedStorage();

    // Get type of storage service.
    if (StorageTypeOfUrl(&request.storage_type, request.azure_url, request.is_emulated_storage) == 0) {
        if (ObjectPathOfUrl(&request.object_path, request.azure_url, request.is_emulated_storage, request.storage_type) == 0) {
            // Parse connection string.
            string connection_string_as_string = util::env::GetEnvVar("AZURE_STORAGE_CONNECTION_STRING");
            request.is_using_connection_string = !connection_string_as_string.empty();
            if (request.is_emulated_storage && !request.is_using_connection_string) {
                GetLogger()->error("Undefined or empty environment variable: AZURE_STORAGE_CONNECTION_STRING.");
                return -1;
            }
            connstr::ConnectionString connection_string;
            if (connstr::ConnectionString::ParseConnectionString(&connectionString, connection_string_as_string, request.is_emulated_storage) == 0) {
                if (connection_string.CheckAgainstUrl(request.azure_url, request.storage_type) == 0) {
                    // Credentials
                    if (request.is_using_connection_string) {
                        request.connection_string_credential = make_shared<Azure::Storage::StorageSharedKeyCredential>(connection_string.sAccountName, connection_string.sAccountKey);
                    } else {
                        request.no_connection_string_credential = make_shared<Azure::Identity::ChainedTokenCredential>(
                            Azure::Identity::ChainedTokenCredential::Sources {
                                std::make_shared<Azure::Identity::EnvironmentCredential>(),  // for Client ID + Client Secret or Certificate environment variables
                                std::make_shared<Azure::Identity::WorkloadIdentityCredential>(),
                                std::make_shared<Azure::Identity::ManagedIdentityCredential>(),
                                std::make_shared<Azure::Identity::AzureCliCredential>()
                            }
                        );
                    }
                    GetLogger()->debug("Just built the following service request:");
                    LogServiceRequest(request);
                    *result = request;
                }
            }
        }
    }

    return -1;
}

string GetServiceUrl(const ServiceRequest &request) {
    ostringstream oss;
    if (request.bEmulated) {  // Emulated BLOB storage
        oss << request.azureUrl.GetScheme() << "://" << request.azureUrl.GetHost()
            << ":" << request.azureUrl.GetPort() << "/"
            << request.blob.sAccountName;
    } else if (request.storageType == BLOB) {  // Real Azure cloud BLOB storage
        oss << request.azureUrl.GetScheme() << "://" << request.azureUrl.GetHost()
            << ":" << request.azureUrl.GetPort();
    } else {  // Real Azure cloud FILE storage
        oss << request.azureUrl.GetScheme() << "://" << request.azureUrl.GetHost()
            << ":" << request.azureUrl.GetPort();
    }
    std::string result = oss.str();
    GetLogger()->debug("Service URL is: {}.", result);
    return result;
}

string GetBlobContainerUrl(const ServiceRequest &request) {
    ostringstream oss;
    oss << GetServiceUrl(request) << "/" << request.blob.sContainer;
    std::string result = oss.str();
    GetLogger()->debug("Blob container URL is: {}.", result);
    return result;
}

Azure::Storage::Blobs::BlobContainerClient GetBlobContainerClient(const ServiceRequest &request) {
    if (request.bUsingConnectionString) {
        return Azure::Storage::Blobs::BlobContainerClient(GetBlobContainerUrl(request), request.connectionStringCredential);
    } else {
        return Azure::Storage::Blobs::BlobContainerClient(GetBlobContainerUrl(request), request.noConnectionStringCredential);
    }
}

vector<Azure::Storage::Blobs::BlobClient> ListBlobs(const ServiceRequest &request) {
    return ResolveBlobsSearchString(GetBlobContainerClient(request), request.blob.sBlob);
}

string GetFileShareUrl(const ServiceRequest &request) {
    ostringstream oss;
    oss << GetServiceUrl(request) << "/" << request.share.sShare;
    string result = oss.str();
    GetLogger()->debug("File share URL is: {}.", result);
    return result;
}

Azure::Storage::Files::Shares::ShareClient
GetShareClient(const ServiceRequest &request) {
    Azure::Storage::Files::Shares::ShareClientOptions opts;
    opts.ShareTokenIntent =
            Azure::Storage::Files::Shares::Models::ShareTokenIntent::Backup;
    if (request.bUsingConnectionString) {
        return Azure::Storage::Files::Shares::ShareClient(
                GetFileShareUrl(request), request.connectionStringCredential, opts);
    } else {
        return Azure::Storage::Files::Shares::ShareClient(
                GetFileShareUrl(request), request.noConnectionStringCredential, opts);
    }
}

Azure::Storage::Files::Shares::ShareDirectoryClient
GetDirClient(const ServiceRequest &request) {
    return GetShareClient(request).GetRootDirectoryClient();
}

vector<Azure::Storage::Files::Shares::ShareDirectoryClient>
ListDirs(const ServiceRequest &request) {
    return ResolveDirsPathRecursively(
            GetDirClient(request),
            queue<string, deque<string>>(
                  deque<string>(request.share.path.begin(), request.share.path.end())));
}

vector<Azure::Storage::Files::Shares::ShareFileClient>
ListFiles(const ServiceRequest &request) {
    return ResolveFilesPathRecursively(
            GetDirClient(request),
            queue<string, deque<string>>(
                  deque<string>(request.share.path.begin(), request.share.path.end())));

}

int GetParentDir(
        Azure::Storage::Files::Shares::ShareDirectoryClient *result,
        const ServiceRequest &request) {
    Azure::Storage::Files::Shares::ShareDirectoryClient dirClient =
            GetDirClient(request);
    vector<string> path = request.share.path;
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
                                 request.azureUrl.GetAbsoluteUrl());
            return -1;
        }

        dirClient = dirClient.GetSubdirectoryClient(sPathFragment);
    }

    if (result)
        *result = dirClient;
    return 0;
}

}