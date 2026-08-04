#include "khiops_driver_azure/util.hpp"
#include <sstream>
#include <queue>
#include <deque>
#include <azure/identity.hpp>
#include <azure/storage/common/storage_exception.hpp>
#include "khiops_driver_common/checks.hpp"
#include "khiops_driver_common/stringify.hpp"
#include "khiops_driver_common/logging.hpp"
#include "khiops_driver_common/util.hpp"
#include "khiops_driver_azure/globalstate.hpp"
#include "khiops_driver_azure/core.hpp"
#include "khiops_driver_azure/connstr.hpp"
#include "khiops_driver_azure/blobpathresolve.hpp"
#include "khiops_driver_azure/sharepathresolve.hpp"

using namespace std;
using namespace khiops_driver_common;

namespace khiops_driver_azure {

int AzureUrlFromString(Azure::Core::Url *result, const string &url) {
    try {
        *result = Azure::Core::Url(url);
    } catch (const exception &) {
        GetLogger()->error("Caught an exception while performing basic URL parsing: URL {} is invalid.", url);
        return -1;
    }
    return 0;
}

int StorageTypeFromHost(StorageType *result, const string &host) {
    if (GetState()->is_emulated_storage) {
        // The emulator supports only blob storage services, not file share storage services.
        *result = BLOB;
    } else if (EndsWith(host, ".blob.core.windows.net")) {
        *result = BLOB;
    } else if (EndsWith(host, ".file.core.windows.net")) {
        *result = FILE_SHARE;
    } else {
        GetLogger()->error("Host {} contains invalid domain.", host);
        return -1;
    }
    return 0;
}

int EmulatedBlobPathFromString(string *account_name, string *blob_container, string *blob, const string &path) {
    //  accountname/container/object
    // OR
    //  accountname/container/object/
    smatch match;
    if (!regex_match(path, match, regex("([^/]+)/([^/]+)/(.+)"))) {
        GetLogger()->error("Invalid emulated storage object path: {}.", path);
        return -1;
    }
    *account_name = match[1].str();
    *blob_container = match[2].str();
    *blob = match[3].str();
    return 0;
}

int BlobPathFromString(string *blob_container, string *blob, const string &path) {
    //  container/object
    // OR
    //  container/object/
    smatch match;
    if (!regex_match(path, match, regex("([^/]+)/(.+)"))) {
        GetLogger()->error("Invalid cloud blob path: {}.", path);
        return -1;
    }
    *blob_container = match[1].str();
    *blob = match[2].str();
    return 0;
}

int FileSharePathFromString(string *file_share, vector<string> *file_path, const string &path) {
    //  share/path/to/a/file
    // OR
    //  share/path/to/a/dir/
    smatch match;
    if (!regex_match(path, match, regex("([^/]+)((?:/[^/]+)+/?)"))) {
        GetLogger()->error("Invalid cloud file path: {}.", path);
        return -1;
    }
    *file_share = match[1].str();
    *file_path = Split(match[2].str(), '/', -1, true);
    return 0;
}

int ResolveFragmentUrls(vector<string> *result, StorageType storage_type, const Azure::Core::Url &azure_url) {
    if (storage_type == BLOB) {
        if (GetState()->is_emulated_storage) {
            string account_name, blob_container, blob;
            if (EmulatedBlobPathFromString(&account_name, &blob_container, &blob, azure_url.GetPath())) return -1;
            *result = ListBlobs(BuildEmulatedServiceUrl(azure_url, account_name), blob_container, blob);
            return 0;
        } else {
            string blob_container, blob;
            if (BlobPathFromString(&blob_container, &blob, azure_url.GetPath())) return -1;
            *result = ListBlobs(BuildServiceUrl(azure_url), blob_container, blob);
            return 0;
        }
    } else {
        string file_share; vector<string> file_path;
        if (FileSharePathFromString(&file_share, &file_path, azure_url.GetPath())) return -1;
        *result = ListFiles(BuildServiceUrl(azure_url), file_share, file_path);
        return 0;
    }
}

int ResolveFragmentUrlsCheckNotEmpty(vector<string> *result, StorageType storage_type, const Azure::Core::Url &azure_url) {
    if (storage_type == BLOB) {
        if (GetState()->is_emulated_storage) {
            string account_name, blob_container, blob;
            if (EmulatedBlobPathFromString(&account_name, &blob_container, &blob, azure_url.GetPath())) return -1;
            if (ListBlobsCheckNotEmpty(result, BuildEmulatedServiceUrl(azure_url, account_name), blob_container, blob)) return -1;
            return 0;
        } else {
            string blob_container, blob;
            if (BlobPathFromString(&blob_container, &blob, azure_url.GetPath())) return -1;
            if (ListBlobsCheckNotEmpty(result, BuildServiceUrl(azure_url), blob_container, blob)) return -1;
            return 0;
        }
    } else {
        string file_share; vector<string> file_path;
        if (FileSharePathFromString(&file_share, &file_path, azure_url.GetPath())) return -1;
        if (ListFilesCheckNotEmpty(result, BuildServiceUrl(azure_url), file_share, file_path)) return -1;
        return 0;
    }
}

#if defined(__linux__)
static Azure::Core::Http::Policies::TransportOptions MakeTransportOptions() {
    Azure::Core::Http::CurlTransportOptions curl_transport_options;
    curl_transport_options.CAInfo = GetState()->certificate_path;
    Azure::Core::Http::Policies::TransportOptions transport_options;
    transport_options.Transport = make_shared<Azure::Core::Http::CurlTransport>(curl_transport_options);
    return transport_options;
}
#endif

static Azure::Storage::Blobs::BlobClientOptions MakeBlobClientOptions() {
    Azure::Storage::Blobs::BlobClientOptions blob_client_options;
    #if defined(__linux__)
    blob_client_options.Transport = MakeTransportOptions();
    #endif
    return blob_client_options;
}

static Azure::Storage::Files::Shares::ShareClientOptions MakeShareClientOptions() {
    Azure::Storage::Files::Shares::ShareClientOptions share_client_options;
    #if defined(__linux__)
    share_client_options.Transport = MakeTransportOptions();
    #endif
    share_client_options.ShareTokenIntent = Azure::Storage::Files::Shares::Models::ShareTokenIntent::Backup;
    return share_client_options;
}

string BuildEmulatedServiceUrl(const string &scheme, const string &host, uint16_t port, const string &account_name) {
    ostringstream oss;
    oss << scheme << "://" << host;
    if (port > 0) oss << ":" << port;
    oss << "/" << account_name;
    return oss.str();
}

string BuildEmulatedServiceUrl(const Azure::Core::Url &azure_url, const string &account_name) {
    return BuildEmulatedServiceUrl(azure_url.GetScheme(), azure_url.GetHost(), azure_url.GetPort(), account_name);
}

string BuildServiceUrl(const string &scheme, const string &host, uint16_t port) {
    ostringstream oss;
    oss << scheme << "://" << host;
    if (port > 0) oss << ":" << port;
    return oss.str();
}

string BuildServiceUrl(const Azure::Core::Url &azure_url) {
    return BuildServiceUrl(azure_url.GetScheme(), azure_url.GetHost(), azure_url.GetPort());
}

string BuildBlobContainerUrl(const string &service_url, const string &container) {
    ostringstream oss;
    oss << service_url << "/" << container;
    return oss.str();
}

Azure::Storage::Blobs::BlobContainerClient GetBlobContainerClient(const string &service_url, const string &container) {
    if (GetState()->is_using_connection_string) {
        return Azure::Storage::Blobs::BlobContainerClient(BuildBlobContainerUrl(service_url, container), GetState()->connection_string_credential, MakeBlobClientOptions());
    } else {
        return Azure::Storage::Blobs::BlobContainerClient(BuildBlobContainerUrl(service_url, container), GetState()->no_connection_string_credential, MakeBlobClientOptions());
    }
}

vector<string> ListBlobs(const string &service_url, const string &container, const string &blob) {
    return ResolveBlobsSearchString(GetBlobContainerClient(service_url, container), blob);
}

int ListBlobsCheckNotEmpty(vector<string> *result, const string &service_url, const string &container, const string &blob) {
    vector<string> objects = ListBlobs(service_url, container, blob);
    if (objects.empty()) {
        GetLogger()->error("No object matches URL {}/{}/{}.", service_url, container, blob);
        return -1;
    }
    *result = std::move(objects);
    return 0;
}

int GetBlobClient(Azure::Storage::Blobs::BlobClient *result, const string &url) {
    if (CheckIsNotGlobbingPattern(url)) return -1;
    if (GetState()->is_using_connection_string) {
        *result = std::move(Azure::Storage::Blobs::BlobClient(url, GetState()->connection_string_credential, MakeBlobClientOptions()));
    } else {
        *result = std::move(Azure::Storage::Blobs::BlobClient(url, GetState()->no_connection_string_credential, MakeBlobClientOptions()));
    }
    return 0;
}

string GetFileShareUrl(const string &service_url, const string &file_share) {
    ostringstream oss;
    oss << service_url << "/" << file_share;
    return oss.str();
}

Azure::Storage::Files::Shares::ShareClient
GetShareClient(const string &service_url, const string &file_share) {
    if (GetState()->is_using_connection_string) {
        return Azure::Storage::Files::Shares::ShareClient(
                GetFileShareUrl(service_url, file_share), GetState()->connection_string_credential, MakeShareClientOptions());
    } else {
        return Azure::Storage::Files::Shares::ShareClient(
                GetFileShareUrl(service_url, file_share), GetState()->no_connection_string_credential, MakeShareClientOptions());
    }
}

Azure::Storage::Files::Shares::ShareDirectoryClient
GetRootDirClient(const string &service_url, const string &file_share) {
    return GetShareClient(service_url, file_share).GetRootDirectoryClient();
}

Azure::Storage::Files::Shares::ShareDirectoryClient
GetDirClient(const string &url) {
    if (GetState()->is_using_connection_string) {
        return Azure::Storage::Files::Shares::ShareDirectoryClient(
                url, GetState()->connection_string_credential, MakeShareClientOptions());
    } else {
        return Azure::Storage::Files::Shares::ShareDirectoryClient(
                url, GetState()->no_connection_string_credential, MakeShareClientOptions());
    }
}

vector<string>
ListDirs(const string &service_url, const string &file_share, const vector<string> &file_path) {
    return ResolveDirsPathRecursively(GetRootDirClient(service_url, file_share), file_path);
}

vector<string>
ListFiles(const string &service_url, const string &file_share, const vector<string> &file_path) {
    return ResolveFilesPathRecursively(GetRootDirClient(service_url, file_share), file_path);
}

int ListFilesCheckNotEmpty(vector<string> *result, const string &service_url, const string &file_share, const vector<string> &file_path) {
    vector<string> objects = ListFiles(service_url, file_share, file_path);
    if (objects.empty()) {
        ostringstream oss;
        oss << service_url << "/" << file_share;
        for (const string &path_segment : file_path) oss << "/" << path_segment;
        GetLogger()->error("No object matches URL {}.", oss.str());
        return -1;
    }
    *result = std::move(objects);
    return 0;
}

int GetParentDir(Azure::Storage::Files::Shares::ShareDirectoryClient *result, const string &service_url, const string &file_share, const vector<string> &file_path) {
    Azure::Storage::Files::Shares::ShareDirectoryClient dir_client = GetRootDirClient(service_url, file_share);
    vector<string> path = file_path;
    path.pop_back();

    for (string path_fragment : path) {
        Azure::Storage::Files::Shares::ListFilesAndDirectoriesOptions opts;
        opts.Prefix = path_fragment;

        bool exists = false;
        for (auto paged_response = dir_client.ListFilesAndDirectories(opts);
                 paged_response.HasPage(); paged_response.MoveToNextPage()) {
            if (find_if(paged_response.Directories.begin(),
                          paged_response.Directories.end(),
                          [path_fragment](const auto &dir_item) {
                            return dir_item.Name == path_fragment;
                          }) != paged_response.Directories.end()) {
                exists = true;
                break;
            }
        }

        if (!exists) {
            GetLogger()->error("Ancestor directory {}/{} does not exist.", dir_client.GetUrl(), path_fragment);
            return -1;
        }

        dir_client = dir_client.GetSubdirectoryClient(path_fragment);
    }

    if (result) *result = dir_client;
    return 0;
}

int GetFileClient(Azure::Storage::Files::Shares::ShareFileClient *result, const std::string &url) {
    if (CheckIsNotGlobbingPattern(url)) return -1;
    if (GetState()->is_using_connection_string) {
        *result = std::move(Azure::Storage::Files::Shares::ShareFileClient(url, GetState()->connection_string_credential, MakeShareClientOptions()));
    } else {
        *result = std::move(Azure::Storage::Files::Shares::ShareFileClient(url, GetState()->no_connection_string_credential, MakeShareClientOptions()));
    }
    return 0;
}

int ReadFragmentToBuffer(size_t *result, StorageType storage_type, const string &fragment_url, const Azure::ETag &version, size_t offset, size_t maxlength, void *buffer) {
    if (CheckNotNull(result, STRINGIFY(result), __func__)) return -1;
    if (CheckNotNull(buffer, STRINGIFY(buffer), __func__)) return -1;

    if (maxlength == 0ULL) {
        *result = 0ULL;
        return 0;
    }

    Azure::Core::Http::HttpRange range;
    range.Offset = static_cast<int64_t>(offset);
    range.Length = static_cast<int64_t>(maxlength);

    unique_ptr<Azure::Core::IO::BodyStream> body_stream;
    try {
        if (storage_type == BLOB) {
            Azure::Storage::Blobs::BlobAccessConditions access_conditions;
            access_conditions.IfMatch = version;
            Azure::Storage::Blobs::DownloadBlobOptions opts;
            opts.Range = range;
            opts.AccessConditions = access_conditions;
            Azure::Storage::Blobs::BlobClient client("");
            if (GetBlobClient(&client, fragment_url) != 0) return -1;
            body_stream = std::move(client.Download(opts).Value.BodyStream);
        } else /* SHARE */ {
            Azure::Storage::Files::Shares::DownloadFileOptions opts;
            opts.Range = range;
            Azure::Storage::Files::Shares::ShareFileClient client("");
            if (GetFileClient(&client, fragment_url) != 0) return -1;
            auto download_result = std::move(client.Download(opts).Value);
            if (download_result.Details.ETag != version) {
                GetLogger()->error("The file has been updated while reading it.");
                return -1;
            }
            body_stream = std::move(download_result.BodyStream);
        }
    } catch (const Azure::Storage::StorageException &exc) {
        if (exc.StatusCode == Azure::Core::Http::HttpStatusCode::PreconditionFailed) {
            GetLogger()->error("The file has been updated while reading it.");
            return -1;
        } else if (exc.StatusCode == Azure::Core::Http::HttpStatusCode::RangeNotSatisfiable) {
            GetLogger()->error("Cannot read after end of file.");
            return -1;
        } else {
            throw;
        }
    }

    const size_t number_of_bytes_read = body_stream->ReadToCount(static_cast<uint8_t *>(buffer), maxlength);
    if (number_of_bytes_read == 0ULL) {
        GetLogger()->error("Cannot read after end of file.");
        return -1;
    }

    *result = number_of_bytes_read;
    return 0;
}

static int ReadFragment(string *result, bool *stopped_on_termchar, StorageType storage_type, const string &fragment_url, const Azure::ETag &version, size_t offset, size_t maxlength, const char *termchar) {
    constexpr size_t HEADER_READ_BUFFER_SIZE = 8ULL * 1024ULL * 1024ULL;
    if (CheckNotNull(result, STRINGIFY(result), __func__)) return -1;
    if (CheckNotNull(stopped_on_termchar, STRINGIFY(stopped_on_termchar), __func__)) return -1;
    if (termchar == nullptr) {
        GetLogger()->error("ReadFragment without terminator is not supported for runtime reads. Use ReadFragmentToBuffer.");
        return -1;
    }
    GetLogger()->debug("Reading a maximum of {} bytes from fragment at URL {} starting at offset {} (can also end if terminator character '{}' is found)...", maxlength, fragment_url, offset, *termchar);
    string content_read = "";
    unique_ptr<Azure::Core::IO::BodyStream> body_stream;
    Azure::ETag previousETag = version;
    size_t number_of_bytes_to_read = maxlength;
    size_t number_of_bytes_read = 0ULL;
    uint8_t *termchar_pos;
    Azure::Core::Http::HttpRange range;
    range.Offset = static_cast<int64_t>(offset);
    vector<uint8_t> buffer(HEADER_READ_BUFFER_SIZE);
    uint8_t *buffer_start = buffer.data();
    while (number_of_bytes_to_read > 0ULL) {
        range.Offset += number_of_bytes_read;
        range.Length = static_cast<int64_t>(min(number_of_bytes_to_read, HEADER_READ_BUFFER_SIZE));
        try {
            if (storage_type == BLOB) {
                Azure::Storage::Blobs::BlobAccessConditions access_conditions;
                access_conditions.IfMatch = previousETag;
                Azure::Storage::Blobs::DownloadBlobOptions opts;
                opts.Range = range;
                opts.AccessConditions = access_conditions;
                Azure::Storage::Blobs::BlobClient client("");
                if (GetBlobClient(&client, fragment_url) != 0) {
                    return -1;
                }
                body_stream = std::move(client.Download(opts).Value.BodyStream);
            } else /* SHARE */ {
                Azure::Storage::Files::Shares::DownloadFileOptions opts;
                opts.Range = range;
                Azure::Storage::Files::Shares::ShareFileClient client("");
                if (GetFileClient(&client, fragment_url) != 0) {
                    return -1;
                }
                auto download_result = std::move(client.Download(opts).Value);
                if (download_result.Details.ETag != previousETag) {
                    GetLogger()->error("The file has been updated while reading it.");
                    return -1;
                }
                body_stream = std::move(download_result.BodyStream);
            }
        } catch (const Azure::Storage::StorageException &exc) {
            if (exc.StatusCode == Azure::Core::Http::HttpStatusCode::PreconditionFailed) {
                GetLogger()->error("The file has been updated while reading it.");
                return -1;
            } else if (exc.StatusCode == Azure::Core::Http::HttpStatusCode::RangeNotSatisfiable) {
                GetLogger()->error("Cannot read after end of file.");
                return -1;
            } else {
                throw;
            }
        }
        number_of_bytes_read = body_stream->ReadToCount(buffer_start, min(number_of_bytes_to_read, HEADER_READ_BUFFER_SIZE));
        if (number_of_bytes_read == 0ULL) {
            // Handle emulator special behavior that gracefully accepts reading beyond file size.
            // Also handle the error case for real cloud storage, even if it should never happen.
            GetLogger()->error("Cannot read after end of file.");
            return -1;
        }
        if (termchar != nullptr) {
            termchar_pos = find(buffer_start, buffer_start + number_of_bytes_read, static_cast<uint8_t>(*termchar));
            if (termchar_pos != buffer_start + number_of_bytes_read) {  // Found terminator character.
                content_read.append(reinterpret_cast<const char *>(buffer_start), termchar_pos + 1 - buffer_start);
                *result = content_read;
                *stopped_on_termchar = true;
                break;
            }
        }
        content_read.append(reinterpret_cast<const char *>(buffer_start), number_of_bytes_read);
        number_of_bytes_to_read -= number_of_bytes_read;
        if (number_of_bytes_to_read == 0ULL) {
            *result = content_read;
            *stopped_on_termchar = false;
            break;
        }
    }
    return 0;
}

int ReadFragment(string *result, bool *stopped_on_termchar, StorageType storage_type, const string &fragment_url, const Azure::ETag &version, size_t offset, size_t maxlength) {
    return ReadFragment(result, stopped_on_termchar, storage_type, fragment_url, version, offset, maxlength, nullptr);
}

int ReadFragment(string *result, bool *stopped_on_termchar, StorageType storage_type, const string &fragment_url, const Azure::ETag &version, size_t offset, size_t maxlength, char termchar) {
    return ReadFragment(result, stopped_on_termchar, storage_type, fragment_url, version, offset, maxlength, &termchar);
}

bool parse_globbing_pattern(const std::string &pattern, std::string *prefix, std::string *suffix) {
    // 1) Output pointers must be non-null
    if (prefix == nullptr || suffix == nullptr) return false;

    // 2) Explicitly forbid "/" and "/*"
    if (pattern == "/" || pattern == "/*") return false;

    // 3) Forbid specific characters anywhere in the pattern
    //    Forbidden: '?', '!', '[', '^'
    if (pattern.find_first_of("?![^") != std::string::npos) return false;

    // 4) Exactly one '*'
    const std::size_t star_pos = pattern.find('*');
    if (star_pos == std::string::npos) return false;
    if (pattern.find('*', star_pos + 1) != std::string::npos) return false;

    // 5) Split
    *prefix = pattern.substr(0, star_pos);
    *suffix = pattern.substr(star_pos + 1);

    // 6) Prefix must be non-empty
    if (prefix->empty()) return false;

    // 7) Prefix must not end with a digit
    {
        const unsigned char c = static_cast<unsigned char>((*prefix)[prefix->size() - 1]);
        if (std::isdigit(c)) return false;
    }

    // 8) If suffix is non-empty, it must not start with a digit
    if (!suffix->empty()) {
        const unsigned char c = static_cast<unsigned char>((*suffix)[0]);
        if (std::isdigit(c)) return false;
    }

    return true;
}


}