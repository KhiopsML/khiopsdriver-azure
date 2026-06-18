#include "khiops_driver_common/driver.h"
#include "khiops_driver_common/logging.hpp"
#include "khiops_driver_common/userfunc_checks.hpp"
#include "khiops_driver_common/returnval.hpp"
#include "khiops_driver_common/util.hpp"
#include "khiops_driver_common/globalstate.hpp"
#include "khiops_driver_azure/filestream.hpp"
#include "khiops_driver_azure/version.hpp"
#include "khiops_driver_azure/core.hpp"
#include "khiops_driver_azure/util.hpp"
#include <fstream>

// Compiling this file means we are currently compiling the driver, so export public functions.
#define CLOUD_STORAGE_DRIVER_EXPORT

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


int driver_connect() {
    const int KO = kOtherFailure;
    CATCH_ALL(
        if (Check_driver_connect()) return KO;
        if (GetState()->is_driver_initialized) {
            GetLogger()->debug("Already connected!");
            return kOtherSuccess;
        }
        if (Initialize()) return KO;
        return kOtherSuccess;
    );
}

int driver_disconnect() {
    const int KO = kOtherFailure;
    CATCH_ALL(
        if (Check_driver_disconnect()) return KO;
        if (GetState()->is_driver_initialized) {
            GetLogger()->debug("Already disconnected!");
            return kOtherSuccess;
        }
        if (Finalize()) return KO;
        return kOtherSuccess;
    );
}

int driver_isConnected() {
    const int KO = kFalse;
    CATCH_ALL(
        if (Check_driver_isConnected()) return KO;
        return GetState()->is_driver_initialized ? kTrue : kFalse;
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

int driver_exist(const char *filename) {
    const int KO = kFalse;
    CATCH_ALL(
        if (Check_driver_exist(filename)) return KO;
        Azure::Core::Url azure_url;
        if (AzureUrlFromString(&azure_url, filename)) return KO;
        StorageType storage_type;
        if (StorageTypeFromHost(&storage_type, azure_url.GetHost())) return KO;
        if (IsDirUrl(filename)) {
            if (storage_type == BLOB) {
                return kTrue;  // There is no such concept as a directory when dealing with blob services.
            } else /* FILE_SHARE */ {
                string file_share; vector<string> file_path;
                if (FileSharePathFromString(&file_share, &file_path, azure_url.GetPath())) return KO;
                return ListDirs(BuildServiceUrl(azure_url), file_share, file_path).empty() ? kFalse : kTrue;
            }
        } else /* not a directory */ {
            vector<string> fragment_urls;
            if (ResolveFragmentUrls(&fragment_urls, storage_type, azure_url)) return KO;
            return fragment_urls.empty() ? kFalse : kTrue;
        }
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
            return kTrue;  // There is no such concept as a directory when dealing with blob services.
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
        FileStreamMode mode = GetState()->open_file_streams.file_streams.at(stream);
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
        return static_cast<long long int>(nread);
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
        return static_cast<long long int>(nwritten);
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
        vector<string> fragment_urls;
        if (ResolveFragmentUrlsCheckNotEmpty(&fragment_urls, storage_type, azure_url)) return KO;
        if (Remove(fragment_urls, storage_type)) return KO;
        return kOtherSuccess;
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
            GetLogger()->info("Making a directory for a blob storage does nothing.");
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
            GetLogger()->info("Removing a directory with a blob storage does nothing.");
            return kOtherSuccess;
        } else /* SHARE */ {
            string file_share; vector<string> file_path;
            if (FileSharePathFromString(&file_share, &file_path, azure_url.GetPath())) return KO;
            vector<string> dirs = ListDirs(BuildServiceUrl(azure_url), file_share, file_path);
            if (dirs.empty()) {
                GetLogger()->error("No directory matches URL {}.", pathname);
                return KO;
            }
            for (const auto &url : dirs) {
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
        size_t buffer_size;
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
            if (GetSystemPreferredBufferSize(&buffer_size)) status = KO;
            else {
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
        size_t buffer_size;
        ifstream ifs;
        size_t ntotalcopied = 0ULL, ntocopy, nread, nwritten, total_size;

        Azure::Core::Url azure_url;
        if (AzureUrlFromString(&azure_url, destfilename)) return KO;
        StorageType storage_type;
        if (StorageTypeFromHost(&storage_type, azure_url.GetHost())) return KO;
        if (FOpenForWriting(reinterpret_cast<void **>(&file_writer), destfilename, storage_type)) return KO;
        if (GetSystemPreferredBufferSize(&buffer_size)) status = KO;
        else {
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
        GetLogger()->error("Not implemented!");
        return KO;
        // return kOtherSuccess;
    );
}