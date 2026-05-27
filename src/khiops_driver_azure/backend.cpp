#include "khiops_driver_common/backend.hpp"
#include "khiops_driver_common/logging.hpp"
#include "khiops_driver_common/util.hpp"
#include "khiops_driver_common/filestream.hpp"
#include "khiops_driver_azure/util.hpp"
#include "khiops_driver_azure/version.hpp"
#include "khiops_driver_azure/servicerequest.hpp"
#include <memory>
#include <algorithm>
#include <fstream>
#include <spdlog/spdlog.h>
#include <azure/core/diagnostics/logger.hpp>
#include <azure/storage/common/storage_exception.hpp>

using namespace std;
using namespace khiops_driver_azure;

namespace khiops_driver_common {

void FileReader::Fragment::FreeVersion() {
    if (this->version != nullptr) {
        delete static_cast<Azure::ETag *>(this->version);
    }
}

spdlog::logger *GetLogger() {
    return GetLogger("azdriver", "AZURE_DRIVER_LOGFILE", "AZURE_DRIVER_LOGLEVEL");
}

int ListFragments(vector<string> *result, const string &url) {
    unique_ptr<ServiceRequest> request;
    if (BuildServiceRequest(&request, url) == 0) {
        *result = std::move(ListBlobsOrFiles(*request));
        return 0;
    }
    return -1;
}

int GetFragmentSizeAndVersion(size_t *size_result, void **version_result, const string &url) {
    unique_ptr<ServiceRequest> request;
    if (BuildServiceRequest(&request, url) == 0) {
        if (request->storage_type == BLOB) {
            Azure::Storage::Blobs::BlobClient client("");
            if (GetBlobClient(&client, *request) == 0) {
                auto blob_properties = std::move(client.GetProperties().Value);
                *size_result = static_cast<size_t>(blob_properties.BlobSize);
                *version_result = static_cast<void *>(new Azure::ETag(blob_properties.ETag.ToString()));
                return 0;
            }
        } else /* SHARE */ {
            Azure::Storage::Files::Shares::ShareFileClient client("");
            if (GetFileClient(&client, *request) == 0) {
                auto file_properties = std::move(client.GetProperties().Value);
                *size_result = static_cast<size_t>(file_properties.FileSize);
                *version_result = static_cast<void *>(new Azure::ETag(file_properties.ETag.ToString()));
                return 0;
            }
        }
    }
    return -1;
}

int ReadFragment(string *result, bool *stopped_on_termchar, const string &url, void *version, size_t offset, size_t maxlength, char termchar) {
    if (result == nullptr || stopped_on_termchar == nullptr || version == nullptr) {
        GetLogger()->error("Null pointer passed to function {}.", __func__);
        return -1;
    }
    if (maxlength == 0ULL) {
        *result = "";
        *stopped_on_termchar = false;
        return 0;
    }
    unique_ptr<ServiceRequest> request;
    string content_read = "";
    unique_ptr<Azure::Core::IO::BodyStream> body_stream;
    size_t buffer_size;
    Azure::ETag previousETag = *static_cast<Azure::ETag *>(version);
    size_t number_of_bytes_to_read = maxlength;
    size_t number_of_bytes_read = 0ULL;
    uint8_t *term_char_pos;
    Azure::Core::Http::HttpRange range;
    range.Offset = static_cast<int64_t>(offset);
    if (BuildServiceRequest(&request, url) == 0) {
        if (GetSystemPreferredBufferSize(&buffer_size) == 0) {
            vector<uint8_t> buffer(buffer_size);
            uint8_t *buffer_start = buffer.data();
            while (number_of_bytes_to_read > 0ULL) {
                range.Offset += number_of_bytes_read;
                range.Length = static_cast<int64_t>(min(number_of_bytes_to_read, buffer_size));
                try {
                    if (request->storage_type == BLOB) {
                        Azure::Storage::Blobs::BlobAccessConditions access_conditions;
                        access_conditions.IfMatch = previousETag;
                        Azure::Storage::Blobs::DownloadBlobOptions opts;
                        opts.Range = range;
                        opts.AccessConditions = access_conditions;
                        Azure::Storage::Blobs::BlobClient client("");
                        if (GetBlobClient(&client, *request) != 0) {
                            return -1;
                        }
                        body_stream = std::move(client.Download(opts).Value.BodyStream);
                    } else /* SHARE */ {
                        Azure::Storage::Files::Shares::DownloadFileOptions opts;
                        opts.Range = range;
                        Azure::Storage::Files::Shares::ShareFileClient client("");
                        if (GetFileClient(&client, *request) != 0) {
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
                    }
                    if (exc.StatusCode == Azure::Core::Http::HttpStatusCode::RangeNotSatisfiable) {
                        GetLogger()->error("Cannot read after end of file.");
                        return -1;
                    }
                    throw;
                }
                number_of_bytes_read = body_stream->ReadToCount(buffer_start, min(number_of_bytes_to_read, buffer_size));
                if (number_of_bytes_read == 0ULL) {
                    // Handle emulator special behavior that gracefully accepts reading beyond file size.
                    // Also handle the error case for real cloud storage, even if it should never happen.
                    GetLogger()->error("Cannot read after end of file.");
                    return -1;
                }
                term_char_pos = find(buffer_start, buffer_start + number_of_bytes_read, termchar);
                if (term_char_pos != buffer_start + number_of_bytes_read) {  // Found terminator character.
                    content_read.append(reinterpret_cast<const char *>(buffer_start), term_char_pos + 1 - buffer_start);
                    *result = content_read;
                    *stopped_on_termchar = true;
                    return 0;
                } else {  // Did not find terminator character.
                    content_read.append(reinterpret_cast<const char *>(buffer_start), number_of_bytes_read);
                    number_of_bytes_to_read -= number_of_bytes_read;
                    if (number_of_bytes_to_read == 0ULL) {
                        *result = content_read;
                        *stopped_on_termchar = false;
                        return 0;
                    }
                }
            }
        }
    }
    return -1;
}

int GetDriverName(string *result) {
    *result = "Azure driver";
    return 0;
}

int GetDriverVersion(string *result) {
    *result = DRIVER_VERSION;
    return 0;
}

int GetDriverScheme(string *result) {
    *result = "https";
    return 0;
}

int IsReadOnly(bool *result) {
    *result = false;
    return 0;
}

int Initialize() {
    // Disable Azure SDK logging.
    // Note: This will not prevent Azure CLI, called as a subprocess by the
    // Azure SDK, to log errors such as "Please run 'az login' to authenticate".
    Azure::Core::Diagnostics::Logger::SetListener([](Azure::Core::Diagnostics::Logger::Level, string const &) {});
    return 0;
}

int Finalize() {
    // Nothing to do.
    return 0;
}

int GetSystemPreferredBufferSize(size_t *result) {
    constexpr size_t DEFAULT_PREFERRED_BUFFER_SIZE = 4ULL * 1024ULL * 1024ULL;
    const string ENVIRONMENT_VARIABLE_NAME = "AZURE_PREFERRED_BUFFER_SIZE";
    static unique_ptr<size_t> system_preferred_buffer_size_memo = nullptr;
    if (system_preferred_buffer_size_memo == nullptr) {
        string environment_variable_preferred_buffer_size = util::env::GetEnvVar(ENVIRONMENT_VARIABLE_NAME);
        if (!environment_variable_preferred_buffer_size.empty()) {
            try {
                *result = stoull(environment_variable_preferred_buffer_size);
                system_preferred_buffer_size_memo = make_unique<size_t>(*result);
                return 0;
            } catch (const invalid_argument &) {
                GetLogger()->warn(
                    "Value {} of environment variable {} is not a valid number. Falling back to default {}...",
                    environment_variable_preferred_buffer_size, ENVIRONMENT_VARIABLE_NAME, DEFAULT_PREFERRED_BUFFER_SIZE
                );
            } catch (const out_of_range &) {
                GetLogger()->warn(
                    "Value {} of environment variable {} is out of range. Falling back to default {}...",
                    environment_variable_preferred_buffer_size, ENVIRONMENT_VARIABLE_NAME, DEFAULT_PREFERRED_BUFFER_SIZE
                );
            }
        }
        *result = DEFAULT_PREFERRED_BUFFER_SIZE;
        system_preferred_buffer_size_memo = make_unique<size_t>(*result);
    } else {
        *result = *system_preferred_buffer_size_memo;
    }
    return 0;
}

int FileExists(bool *result, const string &sFilePathName) {
    unique_ptr<ServiceRequest> request;
    if (BuildServiceRequest(&request, sFilePathName) == 0) {
        *result = !ListBlobsOrFiles(*request).empty();
        return 0;
    }
    return -1;
}

int DirExists(bool *result, const string &sFilePathName) {
    unique_ptr<ServiceRequest> request;
    if (BuildServiceRequest(&request, sFilePathName) == 0) {
        if (request->storage_type == BLOB) {
            *result = true;  // there is no such concept as a directory when dealing with blob services
        } else /* SHARE */ {
            *result = !ListDirs(*request).empty();
        }
        return 0;
    }
    return -1;
}

int GetFileSize(size_t *result, const string &filename) {
    unique_ptr<ServiceRequest> request;
    vector<string> objects;
    if (BuildServiceRequest(&request, filename) == 0 && ListBlobsOrFilesCheckNotEmpty(&objects, *request) == 0) {
        FileReader file_reader;
        PopulateFileReader(&file_reader, filename);
        *result = file_reader.total_size;
        return 0;
    }
    return -1;
}

int FCloseReader(const FileReader &stream) {
    GetLogger()->error("Not implemented!");
    return -1;
}

int FCloseWriter(const FileWriter &stream) {
    GetLogger()->error("Not implemented!");
    return -1;
}

int FRead(size_t *result, void *ptr, const FileReader &file_reader, size_t size, size_t count) {
    GetLogger()->error("Not implemented!");
    return -1;
}

int FSeek(const FileReader &file_reader, long long int offset, int whence) {
    GetLogger()->error("Not implemented!");
    return -1;
}

int FWrite(size_t *result, const FileWriter &file_writer, const void *ptr, size_t size, size_t count) {
    GetLogger()->error("Not implemented!");
    return -1;
}

int FFlush(const FileWriter &file_writer) {
    GetLogger()->error("Not implemented!");
    return -1;
}

int Remove(const string &filename) {
    unique_ptr<ServiceRequest> request;
    vector<string> objects;
    if (BuildServiceRequest(&request, filename) == 0 && ListBlobsOrFilesCheckNotEmpty(&objects, *request) == 0) {
        if (request->storage_type == BLOB) {
            Azure::Storage::Blobs::DeleteBlobOptions opts;
            opts.DeleteSnapshots =
                    Azure::Storage::Blobs::Models::DeleteSnapshotsOption::IncludeSnapshots;
            for (const auto &url : objects) {
                Azure::Storage::Blobs::BlobClient client("");
                if (GetBlobClient(&client, *request, url) != 0) {
                    return -1;
                }
                if (!client.Delete(opts).Value.Deleted) {
                    GetLogger()->error("Failed to delete blob {}.", url);
                    return -1;
                }
            }
        } else /* SHARE */ {
            for (const auto &url : objects) {
                Azure::Storage::Files::Shares::ShareFileClient client("");
                if (GetFileClient(&client, *request, url) != 0) {
                    return -1;
                }
                if (!client.Delete().Value.Deleted) {
                    GetLogger()->error("Failed to delete file {}.", url);
                    return -1;
                }
            }
        }
    }
    return -1;
}

int Mkdir(const string &pathname) {
    unique_ptr<ServiceRequest> request;
    if (BuildServiceRequest(&request, pathname)) {
        return -1;
    }
    if (request->storage_type == BLOB) {
        GetLogger()->info("Making a directory for a blob storage does nothing.");
    } else // SHARE
    {
        string sNewDir = request->object_path.file_path->back();
        Azure::Storage::Files::Shares::ShareDirectoryClient parentDir("");
        if (GetParentDir(&parentDir, *request)) {
            return -1;
        }

        Azure::Storage::Files::Shares::ListFilesAndDirectoriesOptions opts;
        opts.Prefix = sNewDir;
        for (auto pagedResponse = parentDir.ListFilesAndDirectories(opts);
                 pagedResponse.HasPage(); pagedResponse.MoveToNextPage()) {
            if (find_if(pagedResponse.Directories.begin(),
                          pagedResponse.Directories.end(),
                          [sNewDir](const auto &dirItem) {
                            return dirItem.Name == sNewDir;
                          }) != pagedResponse.Directories.end()) {
                GetLogger()->error("Cannot make directory: directory already exists.");
                return -1;
            }
        }

        if (!parentDir.GetSubdirectoryClient(sNewDir).Create().Value.Created) {
            GetLogger()->error("Failed to make directory.");
            return -1;
        }
    }
    return 0;
}

int Rmdir(const string &pathname) {
    unique_ptr<ServiceRequest> request;
    if (BuildServiceRequest(&request, pathname)) {
        return -1;
    }
    if (request->storage_type == BLOB) {
        GetLogger()->info("Removing a directory with a blob storage does nothing.");
    } else // SHARE
    {
        vector<string> dirs = ListDirs(*request);
        if (dirs.empty()) {
            GetLogger()->error("No directory matches URL {}.", pathname);
            return -1;
        }
        for (const auto &url : dirs) {
            if (!GetDirClient(*request, url).Delete().Value.Deleted) {
                GetLogger()->error("Failed to delete directory {}.", url);
                return -1;
            }
        }
    }
    return 0;
}

int DiskFreeSpace(size_t *result, const string &filename) {
    *result = 5ULL * 1024ULL * 1024ULL * 1024ULL * 1024ULL;
    return 0;
}

int CopyToLocal(const string &sourcefilename, const string &destfilename) {
    int status = -1;
    unique_ptr<ServiceRequest> request;
    if (BuildServiceRequest(&request, sourcefilename) == 0) {
        FileReader file_reader;
        if (PopulateFileReader(&file_reader, sourcefilename) == 0) {
            if (file_reader.total_size == 0ULL) {
                GetLogger()->trace("Nothing to copy.");
                status = 0;
            } else {
                size_t buffer_size;
                if (GetSystemPreferredBufferSize(&buffer_size) == 0) {
                    unique_ptr<char[]> buffer = make_unique<char[]>(buffer_size);
                    ofstream ofs(destfilename, ios::binary);
                    if (ofs) {
                        size_t ntotalcopied = 0ULL, nread, ntocopy;
                        while (true) {
                            ntocopy = min(buffer_size, file_reader.total_size - ntotalcopied);
                            GetLogger()->trace("Copying {} bytes from remote to local file...", ntocopy);
                            if (FRead(&nread, buffer.get(), file_reader, 1, ntocopy) == 0) {
                                if (nread == ntocopy) {
                                    ofs.write(buffer.get(), (streamsize)ntocopy);
                                    if (ofs) {
                                        ntotalcopied += ntocopy;
                                        if (ntotalcopied == file_reader.total_size) {
                                            status = 0;
                                            break;
                                        }
                                    } else {
                                        GetLogger()->error("Failed to write to local destination file.");
                                        break;
                                    }
                                } else {
                                    GetLogger()->error("Tried to copy {} bytes but read only {}.", ntocopy, nread);
                                    break;
                                }
                            } else {
                                break;
                            }
                        }
                    } else {
                        GetLogger()->error("Failed to open local destination file.");
                    }
                }
            }
            if (FCloseReader(file_reader) != 0) {
                status = -1;
            }
        }
    }
    return status;
}

int CopyFromLocal(const string &sourcefilename, const string &destfilename) {
    int status = -1;
    unique_ptr<ServiceRequest> request;
    if (BuildServiceRequest(&request, destfilename) == 0) {
        FileWriter file_writer;
        if (OpenForWriting(&file_writer, destfilename) == 0) {
            size_t buffer_size;
            if (GetSystemPreferredBufferSize(&buffer_size) == 0) {
                unique_ptr<char[]> buffer = make_unique<char[]>(buffer_size);
                ifstream ifs(sourcefilename, ios::binary);
                if (ifs) {
                    size_t ntotalcopied = 0ULL, ntocopy, nread, nwritten, total_size;
                    ifs.seekg(0, ios::end);
                    streampos end = ifs.tellg();
                    if (end != streampos(-1)) {
                        ifs.seekg(0, ios::beg);
                        total_size = static_cast<size_t>(end);
                        while (true) {
                            ntocopy = min(buffer_size, total_size - ntotalcopied);
                            GetLogger()->trace("Copying {} bytes from local file to remote...", ntocopy);
                            ifs.read(buffer.get(), ntocopy);
                            if (ifs) {
                                nread = static_cast<size_t>(ifs.gcount());
                                if (nread == ntocopy) {
                                    if (FWrite(&nwritten, file_writer, buffer.get(), 1, ntocopy) == 0) {
                                        if (nwritten == ntocopy) {
                                            ntotalcopied += ntocopy;
                                            if (ntotalcopied == total_size) {
                                                status = 0;
                                                break;
                                            }
                                        } else {
                                            GetLogger()->error("Tried to copy {} bytes but wrote only {}.", ntocopy, nwritten);
                                            break;
                                        }
                                    } else {
                                        break;
                                    }
                                } else {
                                    GetLogger()->error("Tried to copy {} bytes but read only {}.", ntocopy, nread);
                                    break;
                                }
                            } else {
                                GetLogger()->error("Failed to read from local source file.");
                                break;
                            }
                        }
                    }
                } else {
                    GetLogger()->error("Failed to open local source file.");
                }
            }
            if (FCloseWriter(file_writer) != 0) {
                status = -1;
            }
        }
    }
    return status;
}

int Concat(const string &destfilename, const vector<string> &sourcefilenames) {
    GetLogger()->error("Not implemented!");
    return -1;
}

int ComposeMultifile(const string &sDestFilePathName, const vector<string> &sSourceFilePathNames) {
    GetLogger()->error("Not implemented!");
    return -1;
}

}