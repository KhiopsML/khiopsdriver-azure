// Compiling this file means we are currently compiling the driver, so export public functions.
#define CLOUD_STORAGE_DRIVER_EXPORT
#include "khiops_driver_common/driver.h"
#include "khiops_driver_common/logging.hpp"
#include "khiops_driver_common/userfunc_checks.hpp"
#include "khiops_driver_common/returnval.hpp"
#include "khiops_driver_common/util.hpp"
#include "khiops_driver_common/globalstate.hpp"
#include "khiops_driver_azure/globalstate.hpp"
#include "khiops_driver_azure/filestream.hpp"
#include "khiops_driver_azure/version.hpp"
#include "khiops_driver_azure/connstr.hpp"
#include "khiops_driver_azure/core.hpp"
#include "khiops_driver_azure/util.hpp"
#include "khiops_driver_azure/auth.hpp"
#include <fstream>
#include <string>
#include <iomanip>
#include <azure/core/diagnostics/logger.hpp>
#include <azure/identity.hpp>
#include <azure/storage/blobs/block_blob_client.hpp>

// Macro that must be used in all public functions to avoid raising exceptions
// It is variadic just to avoid splitting the code on commas outside of parentheses (otherwise the preprocessor thinks there are multiple macro arguments).
#define CATCH_ALL(...) \
    do { \
        try { \
            __VA_ARGS__ \
        } catch (const exception &exc) { \
            GetLogger()->error("An exception has been raised: {}", exc.what()); \
        } catch (...) { \
            GetLogger()->error("An unknown exception has been raised."); \
        } \
        return KO; \
    } while(0)

using namespace std;
using namespace khiops_driver_common;
using namespace khiops_driver_azure;

namespace {

constexpr size_t INTERNAL_COPY_BUFFER_SIZE = 4ULL * 1024ULL * 1024ULL;

vector<string> ListBlobPrefixUrls(const Azure::Core::Url &azure_url) {
    string service_url;
    string blob_container;
    string blob_prefix;

    if (::khiops_driver_azure::GetState()->is_emulated_storage) {
        string account_name;
        if (EmulatedBlobPathFromString(&account_name, &blob_container, &blob_prefix, azure_url.GetPath())) return {};
        service_url = BuildEmulatedServiceUrl(azure_url, account_name);
    } else {
        if (BlobPathFromString(&blob_container, &blob_prefix, azure_url.GetPath())) return {};
        service_url = BuildServiceUrl(azure_url);
    }

    Azure::Storage::Blobs::ListBlobsOptions opts;
    opts.Prefix = blob_prefix;
    vector<string> matches;
    Azure::Storage::Blobs::BlobContainerClient container_client = GetBlobContainerClient(service_url, blob_container);
    for (auto paged_blob_list = container_client.ListBlobs(opts);
         paged_blob_list.HasPage(); paged_blob_list.MoveToNextPage()) {
        for (const auto &blob_item : paged_blob_list.Blobs) {
            if (!blob_item.IsDeleted) {
                matches.push_back(container_client.GetUrl() + "/" + blob_item.Name);
            }
        }
    }
    return matches;
}

int CreateBlobDirectoryMarker(const char *pathname) {
    void *handle = nullptr;
    if (FOpenForWriting(&handle, pathname, BLOB)) return -1;
    if (FClose(static_cast<FileWriter *>(handle))) return -1;
    return 0;
}

void CollectShareTreeUrls(const Azure::Storage::Files::Shares::ShareDirectoryClient &dir_client,
                          vector<string> *file_urls,
                          vector<string> *dir_urls) {
    for (auto paged_response = dir_client.ListFilesAndDirectories();
         paged_response.HasPage(); paged_response.MoveToNextPage()) {
        for (const auto &file_item : paged_response.Files) {
            file_urls->push_back(dir_client.GetFileClient(file_item.Name).GetUrl());
        }
        for (const auto &dir_item : paged_response.Directories) {
            CollectShareTreeUrls(dir_client.GetSubdirectoryClient(dir_item.Name), file_urls, dir_urls);
        }
    }
    dir_urls->push_back(dir_client.GetUrl());
}

}

const char *driver_getDriverName() {
    const char *const KO = nullptr;
    CATCH_ALL(
        if (Check_driver_getDriverName()) return KO;
        return "Azure driver";
    );
}

const char *driver_getVersion() {
    const char *const KO = nullptr;
    CATCH_ALL(
        if (Check_driver_getVersion()) return KO;
        return DRIVER_VERSION;
    );
}

const char *driver_getScheme() {
    const char *const KO = nullptr;
    CATCH_ALL(
        if (Check_driver_getScheme()) return KO;
        return "https";
    );
}

int driver_isReadOnly() {
    const int KO = kFalse;
    CATCH_ALL(
        if (Check_driver_isReadOnly()) return KO;
        return kFalse;
    );
}

// Not using the CATCH_ALL macro here as MSVC does not understand the embedded #if defined(__linux__) inside macro arguments.
int driver_connect() {
    const int KO = kOtherFailure;
    try {
        if (Check_driver_connect()) return KO;
        if (::khiops_driver_common::GetState()->is_driver_initialized) {
            GetLogger()->debug("Already connected!");
            return kOtherSuccess;
        }

        // Disable Azure SDK logging.
        // Note: This will not prevent Azure CLI, called as a subprocess by the
        // Azure SDK, to log errors such as "Please run 'az login' to authenticate".
        Azure::Core::Diagnostics::Logger::SetListener([](Azure::Core::Diagnostics::Logger::Level, string const &) {});

        ::khiops_driver_common::GetState()->open_file_streams.file_streams.clear();

        string emulated_storage_env_var_val = GetEnvVar("AZURE_EMULATED_STORAGE");
        ::khiops_driver_azure::GetState()->is_emulated_storage = !emulated_storage_env_var_val.empty() && emulated_storage_env_var_val != "false";

        string connection_string_as_string = GetEnvVar("AZURE_STORAGE_CONNECTION_STRING");
        ::khiops_driver_azure::GetState()->is_using_connection_string = !connection_string_as_string.empty();
        
        if (::khiops_driver_azure::GetState()->is_emulated_storage && !::khiops_driver_azure::GetState()->is_using_connection_string) {
            GetLogger()->error("Undefined or empty environment variable: AZURE_STORAGE_CONNECTION_STRING.");
            return KO;
        }
        
        if (::khiops_driver_azure::GetState()->is_using_connection_string) {
            ConnectionString connection_string;
            if (ParseConnectionString(&connection_string, connection_string_as_string, ::khiops_driver_azure::GetState()->is_emulated_storage)) return KO;
            ::khiops_driver_azure::GetState()->connection_string_credential = make_shared<Azure::Storage::StorageSharedKeyCredential>(connection_string.account_name, connection_string.account_key);
        } else /* not using connection string */ {
            ::khiops_driver_azure::GetState()->no_connection_string_credential = make_shared<Azure::Identity::ChainedTokenCredential>(
                Azure::Identity::ChainedTokenCredential::Sources {
                    std::make_shared<Azure::Identity::EnvironmentCredential>(),  // for Client ID + Client Secret or Certificate environment variables
                    std::make_shared<Azure::Identity::WorkloadIdentityCredential>(),
                    std::make_shared<Azure::Identity::ManagedIdentityCredential>(),
                    std::make_shared<Azure::Identity::AzureCliCredential>()
                }
            );
        }

#if defined(__linux__)
        if (FindCertificate(&::khiops_driver_azure::GetState()->certificate_path)) return KO;
#endif

        ::khiops_driver_common::GetState()->is_driver_initialized = true;
        return kOtherSuccess;
    } catch (const exception &exc) {
        GetLogger()->error("An exception has been raised: {}", exc.what());
    } catch (...) { \
        GetLogger()->error("An unknown exception has been raised.");
    }
    return KO;
}

int driver_disconnect() {
    const int KO = kOtherFailure;
    CATCH_ALL(
        if (Check_driver_disconnect()) return KO;
        if (!::khiops_driver_common::GetState()->is_driver_initialized) {
            GetLogger()->debug("Already disconnected!");
            return kOtherSuccess;
        }
        unordered_map<void *, FileStreamMode> handles_to_close = ::khiops_driver_common::GetState()->open_file_streams.file_streams;
        for (const auto &file_stream : handles_to_close) {
            if (file_stream.second == FileStreamMode::READ) {
                if (FClose(static_cast<FileReader *>(file_stream.first))) return KO;
            } else {
                if (FClose(static_cast<FileWriter *>(file_stream.first))) return KO;
            }
        }
        ::khiops_driver_common::GetState()->is_driver_initialized = false;
        return kOtherSuccess;
    );
}

int driver_isConnected() {
    const int KO = kFalse;
    CATCH_ALL(
        if (Check_driver_isConnected()) return KO;
        return ::khiops_driver_common::GetState()->is_driver_initialized ? kTrue : kFalse;
    );
}

long long int driver_getSystemPreferredBufferSize() {
    const long long int KO = kFailure;
    CATCH_ALL(
        if (Check_driver_getSystemPreferredBufferSize()) return KO;
        size_t buffer_size;
        if (GetSystemPreferredBufferSize(&buffer_size)) return KO;
        return static_cast<long long int>(buffer_size);
    );
}

int driver_fileExists(const char *sFilePathName) {
    const int KO = kFalse;
    CATCH_ALL(
        if (Check_driver_fileExists(sFilePathName)) return KO;
        Azure::Core::Url azure_url;
        if (AzureUrlFromString(&azure_url, sFilePathName)) return KO;
        StorageType storage_type;
        if (StorageTypeFromHost(&storage_type, azure_url.GetHost())) return KO;
        vector<string> fragment_urls;
        if (ResolveFragmentUrls(&fragment_urls, storage_type, azure_url)) return KO;
        return fragment_urls.empty() ? kFalse : kTrue;
    );
}

int driver_dirExists(const char *sFilePathName) {
    const int KO = kFalse;
    CATCH_ALL(
        if (Check_driver_dirExists(sFilePathName)) return KO;
        Azure::Core::Url azure_url;
        if (AzureUrlFromString(&azure_url, sFilePathName)) return KO;
        StorageType storage_type;
        if (StorageTypeFromHost(&storage_type, azure_url.GetHost())) return KO;
        if (storage_type == BLOB) {
            return ListBlobPrefixUrls(azure_url).empty() ? kFalse : kTrue;
        } else /* FILE_SHARE */ {
            string file_share; vector<string> file_path;
            if (FileSharePathFromString(&file_share, &file_path, azure_url.GetPath())) return KO;
            return ListDirs(BuildServiceUrl(azure_url), file_share, file_path).empty() ? kFalse : kTrue;
        }
    );
}

long long int driver_getFileSize(const char *filename) {
    const long long int KO = kFailure;
    CATCH_ALL(
        if (Check_driver_getFileSize(filename)) return KO;
        Azure::Core::Url azure_url;
        if (AzureUrlFromString(&azure_url, filename)) return KO;
        StorageType storage_type;
        if (StorageTypeFromHost(&storage_type, azure_url.GetHost())) return KO;
        vector<string> fragment_urls;
        if (ResolveFragmentUrlsCheckNotEmpty(&fragment_urls, storage_type, azure_url)) return KO;
        FileReader file_reader;
        if (PopulateFileReader(&file_reader, storage_type, fragment_urls)) return KO;
        return static_cast<long long int>(file_reader.total_size);
    );
}

void *driver_fopen(const char *filename, char mode) {
    void *const KO = nullptr;
    CATCH_ALL(
        if (Check_driver_fopen(filename, mode)) return KO;
        Azure::Core::Url azure_url;
        if (AzureUrlFromString(&azure_url, filename)) return KO;
        StorageType storage_type;
        if (StorageTypeFromHost(&storage_type, azure_url.GetHost())) return KO;
        void *handle;
        if (mode == 'r') {
            vector<string> fragment_urls;
            if (ResolveFragmentUrlsCheckNotEmpty(&fragment_urls, storage_type, azure_url)) return KO;
            if (FOpenForReading(&handle, fragment_urls, storage_type)) return KO;
            return handle;
        } else if (mode == 'w') {
            if (FOpenForWriting(&handle, filename, storage_type)) return KO;
            return handle;
        } else /* append mode */ {
            if (FOpenForAppending(&handle, filename, storage_type, azure_url)) return KO;
            return handle;
        }
    );
}

int driver_fclose(void *stream) {
    const int KO = kFailure;
    CATCH_ALL(
        if (Check_driver_fclose(stream)) return KO;
        FileStreamMode mode = ::khiops_driver_common::GetState()->open_file_streams.file_streams.at(stream);
        if (mode == FileStreamMode::READ) {
            if (FClose(static_cast<FileReader *>(stream))) return KO;
        } else /* WRITE or APPEND */ {
            if (FClose(static_cast<FileWriter *>(stream))) return KO;
        }
        return kSuccess;
    );
}

long long int driver_fread(void *ptr, size_t size, size_t count, void *stream) {
    const long long int KO = kFailure;
    CATCH_ALL(
        if (Check_driver_fread(ptr, size, count, stream)) return KO;
        size_t nread;
        if (FRead(&nread, ptr, static_cast<FileReader *>(stream), size, count)) return KO;
        // Match C stdlib semantics: return the number of complete elements read.
        return size == 0 ? 0LL : static_cast<long long int>(nread / size);
    );
}

int driver_fseek(void *stream, long long int offset, int whence) {
    const int KO = kFailure;
    CATCH_ALL(
        if (Check_driver_fseek(stream, offset, whence)) return KO;
        if (FSeek(static_cast<FileReader *>(stream), offset, whence)) return KO;
        return kSuccess;
    );
}

const char *driver_getlasterror() {
    const char *const KO = "Error while trying to fetch last error.";
    CATCH_ALL(
        if (Check_driver_getlasterror()) return KO;
        static string last_error;
        last_error = GetLastError();
        return last_error.empty() ? nullptr : last_error.c_str();
    );
}

long long int driver_fwrite(const void *ptr, size_t size, size_t count, void *stream) {
    const long long int KO = kFailure;
    CATCH_ALL(
        if (Check_driver_fwrite(ptr, size, count, stream)) return KO;
        size_t nwritten;
        if (FWrite(&nwritten, static_cast<FileWriter *>(stream), ptr, size, count)) return KO;
        // Match C stdlib semantics: return the number of complete elements written.
        return size == 0 ? 0LL : static_cast<long long int>(nwritten / size);
    );
}

int driver_fflush(void *stream) {
    const int KO = kFailure;
    CATCH_ALL(
        if (Check_driver_fflush(stream)) return KO;
        if (FFlush(static_cast<FileWriter *>(stream))) return KO;
        return kSuccess;
    );
}

int driver_remove(const char *filename) {
    const int KO = kOtherFailure;
    CATCH_ALL(
        if (Check_driver_remove(filename)) return KO;
        Azure::Core::Url azure_url;
        if (AzureUrlFromString(&azure_url, filename)) return KO;
        StorageType storage_type;
        if (StorageTypeFromHost(&storage_type, azure_url.GetHost())) return KO;
        if (string(filename).find('*') == string::npos) {
            if (Remove(vector<string>{filename}, storage_type)) return KO;
            return kOtherSuccess;
        } else /* limited globbing */ {
            string prefix, suffix;
            if (!parse_globbing_pattern(filename, &prefix, &suffix)) {
                GetLogger()->error("Invalid globbing pattern.");
                return KO;
            }
            vector<string> fragment_urls;
            if (ResolveFragmentUrlsCheckNotEmpty(&fragment_urls, storage_type, azure_url)) return KO;
            if (Remove(fragment_urls, storage_type)) return KO;
            return kOtherSuccess;
        }
    );
}

int driver_mkdir(const char *pathname) {
    const int KO = kOtherFailure;
    CATCH_ALL(
        if (Check_driver_mkdir(pathname)) return KO;
        Azure::Core::Url azure_url;
        if (AzureUrlFromString(&azure_url, pathname)) return KO;
        StorageType storage_type;
        if (StorageTypeFromHost(&storage_type, azure_url.GetHost())) return KO;
        if (storage_type == BLOB) {
            if (!ListBlobPrefixUrls(azure_url).empty()) {
                GetLogger()->error("Cannot make directory: directory already exists.");
                return KO;
            }
            if (CreateBlobDirectoryMarker(pathname)) {
                GetLogger()->error("Failed to make directory.");
                return KO;
            }
            return kOtherSuccess;
        } else /* FILE SHARE */ {
            string file_share; vector<string> file_path;
            if (FileSharePathFromString(&file_share, &file_path, azure_url.GetPath())) return KO;
            string new_dir = file_path.back();
            Azure::Storage::Files::Shares::ShareDirectoryClient parent_dir("");
            if (GetParentDir(&parent_dir, BuildServiceUrl(azure_url), file_share, file_path)) return KO;

            Azure::Storage::Files::Shares::ListFilesAndDirectoriesOptions opts;
            opts.Prefix = new_dir;
            for (auto paged_response = parent_dir.ListFilesAndDirectories(opts); paged_response.HasPage(); paged_response.MoveToNextPage()) {
                if (find_if(paged_response.Directories.begin(), paged_response.Directories.end(), [new_dir](const auto &dir_item) { return dir_item.Name == new_dir; }) != paged_response.Directories.end()) {
                    GetLogger()->error("Cannot make directory: directory already exists.");
                    return KO;
                }
            }

            if (!parent_dir.GetSubdirectoryClient(new_dir).Create().Value.Created) {
                GetLogger()->error("Failed to make directory.");
                return KO;
            }
        }
        return kOtherSuccess;
    );
}

int driver_rmdir(const char *pathname) {
    const int KO = kOtherFailure;
    CATCH_ALL(
        if (Check_driver_rmdir(pathname)) return KO;
        Azure::Core::Url azure_url;
        if (AzureUrlFromString(&azure_url, pathname)) return KO;
        StorageType storage_type;
        if (StorageTypeFromHost(&storage_type, azure_url.GetHost())) return KO;
        if (storage_type == BLOB) {
            vector<string> blob_urls = ListBlobPrefixUrls(azure_url);
            if (blob_urls.empty()) {
                GetLogger()->error("No directory matches URL {}.", pathname);
                return KO;
            }
            if (Remove(blob_urls, BLOB)) {
                GetLogger()->error("Failed to delete directory {}.", pathname);
                return KO;
            }
            return kOtherSuccess;
        } else /* SHARE */ {
            string file_share; vector<string> file_path;
            if (FileSharePathFromString(&file_share, &file_path, azure_url.GetPath())) return KO;
            vector<string> dirs = ListDirs(BuildServiceUrl(azure_url), file_share, file_path);
            if (dirs.empty()) {
                GetLogger()->error("No directory matches URL {}.", pathname);
                return KO;
            }
            vector<string> file_urls;
            vector<string> dir_urls;
            for (const auto &url : dirs) {
                CollectShareTreeUrls(GetDirClient(url), &file_urls, &dir_urls);
            }
            for (const auto &url : file_urls) {
                Azure::Storage::Files::Shares::ShareFileClient file_client("");
                if (GetFileClient(&file_client, url)) {
                    GetLogger()->error("Failed to build file client for {}.", url);
                    return KO;
                }
                if (!file_client.Delete().Value.Deleted) {
                    GetLogger()->error("Failed to delete file {}.", url);
                    return KO;
                }
            }
            for (const auto &url : dir_urls) {
                if (!GetDirClient(url).Delete().Value.Deleted) {
                    GetLogger()->error("Failed to delete directory {}.", url);
                    return KO;
                }
            }
        }
        return kOtherSuccess;
    );
}

long long int driver_diskFreeSpace(const char *filename) {
    const long long int KO = kFailure;
    CATCH_ALL(
        if (Check_driver_diskFreeSpace(filename)) return KO;
        return 5ULL * 1024ULL * 1024ULL * 1024ULL * 1024ULL;
    );
}

int driver_copyToLocal(const char *sourcefilename, const char *destfilename) {
    const int KO = kOtherFailure;
    CATCH_ALL(
        if (Check_driver_copyToLocal(sourcefilename, destfilename)) return KO;
        int status = kOtherSuccess;
        Azure::Core::Url azure_url;
        StorageType storage_type;
        vector<string> fragment_urls;
        FileReader *file_reader;
        const size_t buffer_size = INTERNAL_COPY_BUFFER_SIZE;
        unique_ptr<char[]> buffer;
        ofstream ofs;
        size_t ntotalcopied = 0ULL, nread, ntocopy;
        
        if (AzureUrlFromString(&azure_url, sourcefilename)) return KO;
        if (StorageTypeFromHost(&storage_type, azure_url.GetHost())) return KO;
        if (ResolveFragmentUrlsCheckNotEmpty(&fragment_urls, storage_type, azure_url)) return KO;
        if (FOpenForReading(reinterpret_cast<void **>(&file_reader), fragment_urls, storage_type)) return KO;
        if (file_reader->total_size == 0ULL) {
            GetLogger()->trace("Nothing to copy.");
        } else {
            buffer = make_unique<char[]>(buffer_size);
            ofs = ofstream(destfilename, ios::binary);
            if (!ofs) {
                GetLogger()->error("Failed to open local destination file.");
                status = KO;
            } else {
                while (ntotalcopied < file_reader->total_size) {
                    ntocopy = min(buffer_size, file_reader->total_size - ntotalcopied);
                    GetLogger()->trace("Copying {} bytes from remote to local file...", ntocopy);
                    if (FRead(&nread, static_cast<void *>(buffer.get()), file_reader, 1, ntocopy)) {
                        status = KO;
                        break;
                    }
                    if (nread != ntocopy) {
                        GetLogger()->error("Tried to copy {} bytes but read only {}.", ntocopy, nread);
                        status = KO;
                        break;
                    }
                    ofs.write(buffer.get(), (streamsize)ntocopy);
                    if (!ofs) {
                        GetLogger()->error("Failed to write to local destination file.");
                        status = KO;
                        break;
                    }
                    ntotalcopied += ntocopy;
                }
            }
        }
        if (FClose(file_reader)) status = KO;
        return status;
    );
}

int driver_copyFromLocal(const char *sourcefilename, const char *destfilename) {
    const int KO = kOtherFailure;
    CATCH_ALL(
        if (Check_driver_copyFromLocal(sourcefilename, destfilename)) return KO;
        int status = kOtherSuccess;
        FileWriter *file_writer;
        unique_ptr<char[]> buffer;
        const size_t buffer_size = INTERNAL_COPY_BUFFER_SIZE;
        ifstream ifs;
        size_t ntotalcopied = 0ULL, ntocopy, nread, nwritten, total_size;

        Azure::Core::Url azure_url;
        if (AzureUrlFromString(&azure_url, destfilename)) return KO;
        StorageType storage_type;
        if (StorageTypeFromHost(&storage_type, azure_url.GetHost())) return KO;
        if (FOpenForWriting(reinterpret_cast<void **>(&file_writer), destfilename, storage_type)) return KO;
        buffer = make_unique<char[]>(buffer_size);
        ifs = ifstream(sourcefilename, ios::binary);
        if (!ifs) {
            GetLogger()->error("Failed to open local source file.");
            status = KO;
        } else {
            ifs.seekg(0, ios::end);
            streampos end = ifs.tellg();
            if (end == streampos(-1)) {
                GetLogger()->error("Failed to get local source file size.");
                status = KO;
            } else {
                ifs.seekg(0, ios::beg);
                total_size = static_cast<size_t>(end);
                if (total_size == 0ULL) {
                    GetLogger()->trace("Nothing to copy.");
                } else {
                    while (ntotalcopied < total_size) {
                        ntocopy = min(buffer_size, total_size - ntotalcopied);
                        GetLogger()->trace("Copying {} bytes from local file to remote...", ntocopy);
                        ifs.read(buffer.get(), ntocopy);
                        if (!ifs) {
                            GetLogger()->error("Failed to read from local source file.");
                            status = KO;
                            break;
                        }
                        nread = static_cast<size_t>(ifs.gcount());
                        if (nread != ntocopy) {
                            GetLogger()->error("Tried to copy {} bytes but read only {}.", ntocopy, nread);
                            status = KO;
                            break;
                        }
                        if (FWrite(&nwritten, file_writer, static_cast<const void *>(buffer.get()), 1, ntocopy) != 0) {
                            status = KO;
                            break;
                        }
                        if (nwritten != ntocopy) {
                            GetLogger()->error("Tried to copy {} bytes but wrote only {}.", ntocopy, nwritten);
                            status = KO;
                            break;
                        }
                        ntotalcopied += ntocopy;
                    }
                }
            }
        }
        if (FClose(file_writer)) status = KO;
        return status;
    );
}

int driver_concat(const char *destfilename, const char **sourcefilenames, size_t sourcefilecount) {
    const int KO = kOtherFailure;
    CATCH_ALL(
        if (Check_driver_concat(destfilename, sourcefilenames, sourcefilecount)) return KO;
        Azure::Core::Url dest_azure_url;
        if (AzureUrlFromString(&dest_azure_url, destfilename)) return KO;
        StorageType dest_storage_type;
        if (StorageTypeFromHost(&dest_storage_type, dest_azure_url.GetHost())) return KO;
        vector<Azure::Core::Url> source_azure_urls(sourcefilecount);
        vector<StorageType> source_storage_types(sourcefilecount);
        if (dest_storage_type == BLOB) {
            vector<string> source_blob_containers(sourcefilecount);
            vector<string> source_blobs(sourcefilecount);
            for (size_t i = 0ULL; i < sourcefilecount; i++) {
                if (AzureUrlFromString(&source_azure_urls[i], sourcefilenames[i])) return KO;
                if (StorageTypeFromHost(&source_storage_types[i], source_azure_urls[i].GetHost())) return KO;
                if (source_storage_types[i] != BLOB) {
                    GetLogger()->error("Source's storage type (blob/file) does not match destination's storage type.");
                    return KO;
                }
                if (BlobPathFromString(&source_blob_containers[i], &source_blobs[i], source_azure_urls[i].GetPath())) return KO;
            }
            if (ConcatBlob(destfilename, vector<string>(sourcefilenames, sourcefilenames + sourcefilecount), source_blob_containers, source_blobs)) return KO;
        } else /* FILE SHARE */ {
            vector<string> source_file_shares(sourcefilecount);
            vector<vector<string>> source_file_paths(sourcefilecount);
            for (size_t i = 0ULL; i < sourcefilecount; i++) {
                if (AzureUrlFromString(&source_azure_urls[i], sourcefilenames[i])) return KO;
                if (StorageTypeFromHost(&source_storage_types[i], source_azure_urls[i].GetHost())) return KO;
                if (source_storage_types[i] != FILE_SHARE) {
                    GetLogger()->error("Source's storage type (blob/file) does not match destination's storage type.");
                    return KO;
                }
                if (FileSharePathFromString(&source_file_shares[i], &source_file_paths[i], source_azure_urls[i].GetPath())) return KO;
            }
            if (ConcatFileShare(destfilename, vector<string>(sourcefilenames, sourcefilenames + sourcefilecount), source_file_shares, source_file_paths)) return KO;
        }
        return kOtherSuccess;
    );
}

int driver_composeMultifile(const char *sDestFilePathName, const char **sSourceFilePathNames, size_t nSourceFileCount) {
    const int KO = kOtherFailure;
    CATCH_ALL(
        if (Check_driver_composeMultifile(sDestFilePathName, sSourceFilePathNames, nSourceFileCount)) return KO;

        // ---- Local autonomous helpers --------------------------------------

        auto is_relative_path = [](const char *p) -> bool {
            if (p == NULL) return false;
            const std::string s(p);
            if (s.empty()) return false;
            if (s.find("://") != std::string::npos) return false;
            if (s[0] == '/') return false;
            return true;
        };

        auto generate_sequence_number = [](size_t i) -> std::string {
            std::ostringstream oss;
            oss << std::setfill('0') << std::setw(12) << i;
            return oss.str();
        };

        auto strip_container_prefix_if_present = [](const std::string &relative_path, const std::string &container) -> std::string {
            const std::string prefix = container + "/";
            if (relative_path.size() > prefix.size() &&
                relative_path.compare(0, prefix.size(), prefix) == 0) {
                return relative_path.substr(prefix.size());
            }
            return relative_path;
        };

        // --------------------------------------------------------------------

        std::string prefix;
        std::string suffix;
        if (!parse_globbing_pattern(std::string(sDestFilePathName), &prefix, &suffix)) {
            GetLogger()->error("Invalid globbing pattern.");
            return KO;
        }

        Azure::Core::Url dest_azure_url;
        if (AzureUrlFromString(&dest_azure_url, prefix)) {
            GetLogger()->error("Invalid destination prefix URL.");
            return KO;
        }

        StorageType dest_storage_type;
        if (StorageTypeFromHost(&dest_storage_type, dest_azure_url.GetHost())) return KO;

        // Parse destination base object/path
        std::string dest_blob_container;
        std::string dest_blob_base;
        std::string dest_file_share;
        std::vector<std::string> dest_file_path;
        std::string dest_service_url;
        std::string dest_base_path;

        if (dest_storage_type == BLOB) {
            if (::khiops_driver_azure::GetState()->is_emulated_storage) {
                std::string account_name;
                if (EmulatedBlobPathFromString(&account_name, &dest_blob_container, &dest_blob_base, dest_azure_url.GetPath())) return KO;
                dest_service_url = BuildEmulatedServiceUrl(dest_azure_url, account_name);
            } else {
                if (BlobPathFromString(&dest_blob_container, &dest_blob_base, dest_azure_url.GetPath())) return KO;
                dest_service_url = BuildServiceUrl(dest_azure_url);
            }
            dest_base_path = dest_blob_base;
        } else {
            if (FileSharePathFromString(&dest_file_share, &dest_file_path, dest_azure_url.GetPath())) return KO;
            if (dest_file_path.empty()) {
                GetLogger()->error("Invalid destination file-share path.");
                return KO;
            }

            std::ostringstream oss;
            oss << dest_file_path[0];
            for (size_t i = 1ULL; i < dest_file_path.size(); ++i) {
                oss << "/" << dest_file_path[i];
            }
            dest_base_path = oss.str();
            dest_service_url = BuildServiceUrl(dest_azure_url);
        }

        // Validate sources are relative
        for (size_t i = 0ULL; i < nSourceFileCount; ++i) {
            if (!is_relative_path(sSourceFilePathNames[i])) {
                GetLogger()->error("Source file path must be relative (no scheme allowed): {}",
                                   sSourceFilePathNames[i] ? sSourceFilePathNames[i] : "<null>");
                return KO;
            }
            GetLogger()->debug("Source #{}: {}", i, sSourceFilePathNames[i]);
        }

        bool failure_detected = false;

        for (size_t i = 0ULL; i < nSourceFileCount; ++i) {
            const std::string source_relative_raw = sSourceFilePathNames[i];
            const std::string seq = generate_sequence_number(i);

            std::ostringstream new_name_oss;
            new_name_oss << dest_base_path << seq << suffix;
            const std::string new_relative = new_name_oss.str();

            if (dest_storage_type == BLOB) {
                const std::string source_relative_blob =
                    strip_container_prefix_if_present(source_relative_raw, dest_blob_container);

                GetLogger()->debug("Renaming {} to {}", source_relative_blob, new_relative);

                // Build full source and destination URLs
                std::ostringstream src_url_oss;
                src_url_oss << dest_service_url << "/" << dest_blob_container << "/" << source_relative_blob;
                const std::string source_url = src_url_oss.str();

                std::ostringstream dst_url_oss;
                dst_url_oss << dest_service_url << "/" << dest_blob_container << "/" << new_relative;
                const std::string dest_url = dst_url_oss.str();

                Azure::Storage::Blobs::BlobClient src_client("");
                Azure::Storage::Blobs::BlobClient dst_client("");

                if (GetBlobClient(&src_client, source_url)) {
                    GetLogger()->error("Failed to get source blob client for {}", source_url);
                    failure_detected = true;
                    continue;
                }
                if (GetBlobClient(&dst_client, dest_url)) {
                    GetLogger()->error("Failed to get destination blob client for {}", dest_url);
                    failure_detected = true;
                    continue;
                }

                try {
                    // Build authenticated source URI/header
                    Auth source_auth;
                    if (BuildBlobAuth(&source_auth, source_url, dest_blob_container, source_relative_blob)) {
                        failure_detected = true;
                        continue;
                    }

                    // Server-side "rename" using one staged block copied from source URI
                    Azure::Storage::Blobs::BlockBlobClient dst_block_blob_client = dst_client.AsBlockBlobClient();

                    std::vector<std::string> block_ids;
                    {
                        std::ostringstream oss;
                        oss << std::setfill('0') << std::setw(64) << 0;
                        const std::string block_id_in_base10 = oss.str();
                        std::vector<uint8_t> block_id_bytes(block_id_in_base10.begin(), block_id_in_base10.end());
                        const std::string block_id_in_base64 = Azure::Core::Convert::Base64Encode(block_id_bytes);
                        block_ids.push_back(block_id_in_base64);

                        Azure::Storage::Blobs::StageBlockFromUriOptions stage_opts;
                        if (source_auth.HasHeader()) {
                            stage_opts.SourceAuthorization = source_auth.sAuthHeader;
                        }

                        dst_block_blob_client.StageBlockFromUri(block_id_in_base64, source_auth.sUriAuth, stage_opts);
                    }

                    dst_block_blob_client.CommitBlockList(block_ids);

                    if (!src_client.Delete().Value.Deleted) {
                        GetLogger()->error("Failed to delete original blob {}", source_url);
                        failure_detected = true;
                    }
                } catch (const Azure::Core::RequestFailedException &exc) {
                    GetLogger()->error("Failed to rename '{}' to '{}': {}", source_relative_blob, new_relative, exc.what());
                    failure_detected = true;
                    continue;
                }
            } else {
                // FILE SHARE case
                const std::string source_relative = strip_container_prefix_if_present(source_relative_raw, dest_file_share);

                GetLogger()->debug("Renaming {} to {}", source_relative, new_relative);

                std::vector<std::string> source_parts = Split(source_relative, '/', -1, true);
                if (source_parts.empty()) {
                    GetLogger()->error("Invalid source relative path {}", source_relative);
                    failure_detected = true;
                    continue;
                }

                // Source URL
                std::ostringstream src_url_oss;
                src_url_oss << dest_service_url << "/" << dest_file_share;
                for (size_t p = 0ULL; p < source_parts.size(); ++p) {
                    src_url_oss << "/" << source_parts[p];
                }
                const std::string source_url = src_url_oss.str();

                // Destination URL
                std::vector<std::string> dest_parts = Split(new_relative, '/', -1, true);
                if (dest_parts.empty()) {
                    GetLogger()->error("Invalid destination relative path {}", new_relative);
                    failure_detected = true;
                    continue;
                }

                std::ostringstream dst_url_oss;
                dst_url_oss << dest_service_url << "/" << dest_file_share;
                for (size_t p = 0ULL; p < dest_parts.size(); ++p) {
                    dst_url_oss << "/" << dest_parts[p];
                }
                const std::string dest_url = dst_url_oss.str();

                // Ensure destination parent directory exists
                if (dest_parts.size() > 1ULL) {
                    std::vector<std::string> parent_path(dest_parts.begin(), dest_parts.end() - 1);
                    Azure::Storage::Files::Shares::ShareDirectoryClient parent_dir("");
                    if (GetParentDir(&parent_dir, dest_service_url, dest_file_share, parent_path)) {
                        GetLogger()->error("Destination parent directory does not exist for {}", dest_url);
                        failure_detected = true;
                        continue;
                    }
                }

                Azure::Storage::Files::Shares::ShareFileClient src_client("");
                Azure::Storage::Files::Shares::ShareFileClient dst_client("");

                if (GetFileClient(&src_client, source_url)) {
                    GetLogger()->error("Failed to get source file client for {}", source_url);
                    failure_detected = true;
                    continue;
                }
                if (GetFileClient(&dst_client, dest_url)) {
                    GetLogger()->error("Failed to get destination file client for {}", dest_url);
                    failure_detected = true;
                    continue;
                }

                try {
                    const auto props = src_client.GetProperties().Value;
                    const int64_t source_size = props.FileSize;

                    dst_client.Create(source_size);

                    // Auth for source URI
                    Auth source_auth;
                    if (BuildFileShareAuth(&source_auth, source_url, dest_file_share, source_parts)) {
                        failure_detected = true;
                        continue;
                    }

                    constexpr int64_t MAX_SOURCE_SIZE = 4LL * 1024LL * 1024LL;
                    for (int64_t offset_in_source = 0LL; offset_in_source < source_size; offset_in_source += MAX_SOURCE_SIZE) {
                        const int64_t to_copy = std::min<int64_t>(source_size - offset_in_source, MAX_SOURCE_SIZE);
                        Azure::Core::Http::HttpRange range{offset_in_source, to_copy};
                        Azure::Storage::Files::Shares::UploadFileRangeFromUriOptions opts;
                        if (source_auth.HasHeader()) {
                            opts.SourceAuthorization = source_auth.sAuthHeader;
                        }
                        dst_client.UploadRangeFromUri(offset_in_source, source_auth.sUriAuth, range, opts);
                    }

                    if (!src_client.Delete().Value.Deleted) {
                        GetLogger()->error("Failed to delete original file {}", source_url);
                        failure_detected = true;
                    }
                } catch (const Azure::Core::RequestFailedException &exc) {
                    GetLogger()->error("Failed to rename '{}' to '{}': {}", source_relative, new_relative, exc.what());
                    failure_detected = true;
                    continue;
                }
            }
        }

        return failure_detected ? KO : kOtherSuccess;
    );
}
