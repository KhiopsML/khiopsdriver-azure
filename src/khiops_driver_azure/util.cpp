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

bool IsEmulatedStorage() {
    string sEmulatedStorageEnvVarVal = util::env::GetEnvVar("AZURE_EMULATED_STORAGE");
    return !sEmulatedStorageEnvVarVal.empty() && sEmulatedStorageEnvVarVal != "false";
}

int ParseUrl(ServiceRequest *result, const string &sUrl) {
    // Basic URL parsing
    Azure::Core::Url url;
    try {
        url = Azure::Core::Url(sUrl);
    } catch (const exception &) {
        GetLogger()->error("Caught an exception while performing basic URL parsing: URL {} is invalid.", sUrl);
        return -1;
    }
    const string &sPath = url.GetPath();

    // Determine some properties about the requested resource
    bool bDir = util::str::EndsWith(sPath, "/");
    bool bIsEmulatedStorage = IsEmulatedStorage();
    bool bAzureBlobUrl = util::str::EndsWith(url.GetHost(), ".blob.core.windows.net");
    bool bAzureFileUrl = !bAzureBlobUrl && util::str::EndsWith(url.GetHost(), ".file.core.windows.net");
    if (!bIsEmulatedStorage && !bAzureBlobUrl && !bAzureFileUrl) {
        GetLogger()->error("URL {} contains invalid domain.", sUrl);
        return -1;
    }

    // Determine storage type
    StorageType storageType;
    if (bIsEmulatedStorage) {
        storageType = BLOB; // The emulator supports only blob storages
    } else if (bAzureBlobUrl) {
        storageType = BLOB;
    } else {
        storageType = SHARE;
    }

    // Connection string parsing
    string sConnectionString = util::env::GetEnvVar("AZURE_STORAGE_CONNECTION_STRING");
    bool bConnectionStringDefined = !sConnectionString.empty();
    connstr::ConnectionString connectionString;
    if (bConnectionStringDefined) {
        if (connstr::ConnectionString::ParseConnectionString(&connectionString, sConnectionString, bIsEmulatedStorage)) {
            return -1;
        }
        if (connectionString.CheckAgainstUrl(url, storageType)) {
            return -1;
        }
    }

    // Credentials
    shared_ptr<Azure::Storage::StorageSharedKeyCredential> connectionStringCredential;
    shared_ptr<Azure::Core::Credentials::TokenCredential> noConnectionStringCredential;
    if (bConnectionStringDefined) {
        connectionStringCredential = make_shared<Azure::Storage::StorageSharedKeyCredential>(connectionString.sAccountName, connectionString.sAccountKey);
    } else {
        noConnectionStringCredential = make_shared<Azure::Identity::ChainedTokenCredential>(
            Azure::Identity::ChainedTokenCredential::Sources {
                std::make_shared<Azure::Identity::EnvironmentCredential>(),  // for Client ID + Client Secret or Certificate environment variables
                std::make_shared<Azure::Identity::WorkloadIdentityCredential>(),
                std::make_shared<Azure::Identity::ManagedIdentityCredential>(),
                std::make_shared<Azure::Identity::AzureCliCredential>()
            }
        );
    }

    // Split stored object path
    if (bIsEmulatedStorage) // Emulated BLOB storage
    {
        if (!bConnectionStringDefined) {
            GetLogger()->error("Undefined or empty environment variable: AZURE_STORAGE_CONNECTION_STRING.");
            return -1;
        }
        smatch match;
        if (!regex_match(
                    sPath, match,
                    regex("([^/]+)/([^/]+)/(.+)"))) //  accountname/container/object  or
                                                    //  accountname/container/object/
        {
            GetLogger()->error("Invalid emulated storage object path: {}.", sPath);
            return -1;
        }
        *result = ServiceRequest(url, bIsEmulatedStorage, BLOB, bDir, BlobInfo {match[1].str(), match[2].str(), match[3].str()}, connectionStringCredential);
    } else if (bAzureBlobUrl) { // Real Azure cloud BLOB storage
        smatch match;
        if (!regex_match(sPath, match, regex("([^/]+)/(.+)"))) //  container/object  or  container/object/
        {
            GetLogger()->error("Invalid cloud blob path: {}.", sPath);
            return -1;
        }
        if (bConnectionStringDefined) {
            *result = ServiceRequest(url, bIsEmulatedStorage, BLOB, bDir, BlobInfo{string(), match[1].str(), match[2].str()}, connectionStringCredential);
        } else {
            *result = ServiceRequest(url, bIsEmulatedStorage, BLOB, bDir, BlobInfo{string(), match[1].str(), match[2].str()}, noConnectionStringCredential);
        }
    } else { // Real Azure cloud SHARE storage
        smatch match;
        if (!regex_match(sPath, match, regex("([^/]+)((?:/[^/]+)+/?)")))  //  share/path/to/a/file  or  share/path/to/a/dir/
        {
            GetLogger()->error("Invalid cloud file path: {}.", sPath);
            return -1;
        }
        vector<string> fileOrDirPath = util::str::Split(match[2].str(), '/', -1, true);
        if (bConnectionStringDefined) {
            *result = ServiceRequest(url, bIsEmulatedStorage, SHARE, bDir, ShareInfo{match[1].str(), fileOrDirPath}, connectionStringCredential);
        } else {
            *result = ServiceRequest(url, bIsEmulatedStorage, SHARE, bDir, ShareInfo{match[1].str(), fileOrDirPath}, noConnectionStringCredential);
        }
    }
    return 0;
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