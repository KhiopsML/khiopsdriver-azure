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
#include <limits>
#include <cstring>
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

static int ReadFragment(string *result, bool *stopped_on_termchar, const string &url, void *version, size_t offset, size_t maxlength, const char *termchar) {
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
    uint8_t *termchar_pos;
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
                if (termchar != nullptr) {
                    termchar_pos = find(buffer_start, buffer_start + number_of_bytes_read, static_cast<uint8_t>(*termchar));
                    if (termchar_pos != buffer_start + number_of_bytes_read) {  // Found terminator character.
                        content_read.append(reinterpret_cast<const char *>(buffer_start), termchar_pos + 1 - buffer_start);
                        *result = content_read;
                        *stopped_on_termchar = true;
                        return 0;
                    }
                }
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
    return -1;
}

int ReadFragment(string *result, bool *stopped_on_termchar, const string &url, void *version, size_t offset, size_t maxlength) {
    return ReadFragment(result, stopped_on_termchar, url, version, offset, maxlength, nullptr);
}

int ReadFragment(string *result, bool *stopped_on_termchar, const string &url, void *version, size_t offset, size_t maxlength, char termchar) {
    return ReadFragment(result, stopped_on_termchar, url, version, offset, maxlength, &termchar);
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
    // Nothing to do.
    return 0;
}

int FCloseWriter(const FileWriter &stream) {
    if (FFlush(stream) == 0) return 0;
    else return -1;
}

int FRead(size_t *result, void *ptr, FileReader *file_reader, size_t size, size_t count) {
    if (result == nullptr || ptr == nullptr || file_reader == nullptr) { GetLogger()->error("Null pointer passed to function {}.", __func__); return -1; }
    if (size != 0 && count > numeric_limits<size_t>::max() / size) { GetLogger()->error("Number of bytes to read asked overflows."); return -1; }

    const size_t ntotaltoread = size * count;
    size_t nlefttoread = ntotaltoread, ntotalread = 0ULL, ntoread, nread, offset_inside_first_fragment_to_read, fragment_remote_offset, fragment_index;
    string globalread, read;
    bool stopped_on_term_char, first_fragment_to_read = true;

    if (ntotaltoread == 0) { *result = 0ULL; return 0; }
    if (file_reader->current_position == file_reader->total_size) { GetLogger()->error("Reading after end of file."); return -1; }
    if (FragmentIndexOfUserOffset(&fragment_index, *file_reader, file_reader->current_position) != 0) return -1;
    while (nlefttoread != 0ULL && file_reader->current_position != file_reader->total_size) {
        const FileReader::Fragment &fragment = file_reader->fragments[fragment_index];
        if (first_fragment_to_read) {
            offset_inside_first_fragment_to_read = file_reader->current_position - fragment.user_offset;
            fragment_remote_offset = (fragment_index == 0ULL ? 0ULL : file_reader->header_length) + offset_inside_first_fragment_to_read;
            ntoread = min(nlefttoread, fragment.content_size - offset_inside_first_fragment_to_read);
        } else {
            fragment_remote_offset = file_reader->header_length;
            ntoread = min(nlefttoread, fragment.content_size);
        }
        if (ReadFragment(&read, &stopped_on_term_char, fragment.url, fragment.version, fragment_remote_offset, ntoread) != 0) {
            return -1;
        }
        nread = read.size();
        if (nread != ntoread) { GetLogger()->error("Failed to read."); return -1; }
        ntotalread += nread;
        globalread.append(read);
        nlefttoread -= nread;
        fragment_index++;
        first_fragment_to_read = false;
        file_reader->current_position += nread;
    }
    *result = ntotalread;
    memcpy(ptr, globalread.data(), ntotalread);
    return 0;
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
    int status;
    unique_ptr<ServiceRequest> request;
    FileReader file_reader;
    size_t buffer_size;
    unique_ptr<char[]> buffer;
    ofstream ofs;
    size_t ntotalcopied = 0ULL, nread, ntocopy;

    if (BuildServiceRequest(&request, sourcefilename) != 0) return -1;
    if (PopulateFileReader(&file_reader, sourcefilename) != 0) return -1;
    if (file_reader.total_size == 0ULL) { GetLogger()->trace("Nothing to copy."); return 0; }
    if (GetSystemPreferredBufferSize(&buffer_size) != 0) return -1;
    buffer = make_unique<char[]>(buffer_size);
    ofs = ofstream(destfilename, ios::binary);
    if (!ofs) { GetLogger()->error("Failed to open local destination file."); return -1; }
    status = 0;
    while (ntotalcopied < file_reader.total_size) {
        ntocopy = min(buffer_size, file_reader.total_size - ntotalcopied);
        GetLogger()->trace("Copying {} bytes from remote to local file...", ntocopy);
        if (FRead(&nread, buffer.get(), file_reader, 1, ntocopy) != 0) { status = -1; break; }
        if (nread != ntocopy) { GetLogger()->error("Tried to copy {} bytes but read only {}.", ntocopy, nread); status = -1; break; }
        ofs.write(buffer.get(), (streamsize)ntocopy);
        if (!ofs) { GetLogger()->error("Failed to write to local destination file."); status = -1; break; }
        ntotalcopied += ntocopy;
    }
    if (FCloseReader(file_reader) != 0) return -1;
    return status;
}

int CopyFromLocal(const string &sourcefilename, const string &destfilename) {
    int status;
    unique_ptr<ServiceRequest> request;
    FileWriter file_writer;
    unique_ptr<char[]> buffer;
    size_t buffer_size;
    ifstream ifs;
    size_t ntotalcopied = 0ULL, ntocopy, nread, nwritten, total_size;

    if (BuildServiceRequest(&request, destfilename) != 0) return -1;
    if (OpenForWriting(&file_writer, destfilename) != 0) return -1;
    if (GetSystemPreferredBufferSize(&buffer_size) != 0) return -1;
    buffer = make_unique<char[]>(buffer_size);
    ifs = ifstream(sourcefilename, ios::binary);
    if (!ifs) { GetLogger()->error("Failed to open local source file."); return -1; }
    ifs.seekg(0, ios::end);
    streampos end = ifs.tellg();
    if (end == streampos(-1)) { GetLogger()->error("Failed to get local source file size."); return -1; }
    ifs.seekg(0, ios::beg);
    total_size = static_cast<size_t>(end);
    if (total_size == 0ULL) { GetLogger()->trace("Nothing to copy."); return 0; }
    status = 0;
    while (ntotalcopied < total_size) {
        ntocopy = min(buffer_size, total_size - ntotalcopied);
        GetLogger()->trace("Copying {} bytes from local file to remote...", ntocopy);
        ifs.read(buffer.get(), ntocopy);
        if (!ifs) { GetLogger()->error("Failed to read from local source file."); status = -1; break; }
        nread = static_cast<size_t>(ifs.gcount());
        if (nread != ntocopy) { GetLogger()->error("Tried to copy {} bytes but read only {}.", ntocopy, nread); status = -1; break; }
        if (FWrite(&nwritten, file_writer, buffer.get(), 1, ntocopy) != 0) { status = -1; break; }
        if (nwritten != ntocopy) { GetLogger()->error("Tried to copy {} bytes but wrote only {}.", ntocopy, nwritten); status = -1; break; }
        ntotalcopied += ntocopy;
    }
    if (FCloseWriter(file_writer) != 0) return -1;
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