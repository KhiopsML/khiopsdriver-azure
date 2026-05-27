#include "khiops_driver_common/backend.hpp"
#include "khiops_driver_common/logging.hpp"
#include "khiops_driver_common/util.hpp"
#include "khiops_driver_common/filestream.hpp"
#include "khiops_driver_azure/util.hpp"
#include "khiops_driver_azure/version.hpp"
#include "khiops_driver_azure/servicerequest.hpp"
#include <memory>
#include <algorithm>
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

int FOpen(FileStream &stream, const string &filename) {
    if (stream.mode == FileStream::Mode::READ) {
    } else if (stream.mode == FileStream::Mode::WRITE) {
    } else if (stream.mode == FileStream::Mode::APPEND) {
    } else {
        GetLogger()->error("Invalid file stream mode.");
        return -1;
    }
}

int FClose(const FileStream &stream) {
    GetLogger()->error("Not implemented!");
    return -1;
}

int FRead(size_t *result, void *ptr, size_t size, size_t count, FileStream &stream) {
    GetLogger()->error("Not implemented!");
    return -1;
}

int FSeek(FileStream &stream, long long int offset, int whence) {
    GetLogger()->error("Not implemented!");
    return -1;
}

int FWrite(size_t *result, const void *ptr, size_t size, size_t count, const FileStream &stream) {
    GetLogger()->error("Not implemented!");
    return -1;
}

int FFlush(const FileStream &stream) {
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
        auto dirs = ListDirs(*request);
        if (dirs.empty()) {
            GetLogger()->error("No directory matches URL {}.", pathname);
            return -1;
        }
        for (const auto &dir : dirs) {
            const string sDirUrl = dir.GetUrl();
            if (!dir.Delete().Value.Deleted) {
                GetLogger()->error("Failed to delete directory {}.", sDirUrl);
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
    unique_ptr<ServiceRequest> request;
    if (BuildServiceRequest(&request, sourcefilename)) {
        return -1;
    }
    FileStream *readerPtr;
    if (OpenForReading(&readerPtr, sourcefilename)) {
        return -1;
    }
    size_t system_preferred_buffer_size;
    if (GetSystemPreferredBufferSize(&system_preferred_buffer_size) != 0) {
        return -1;
    }
    unique_ptr<char[]> buffer = make_unique<char[]>(system_preferred_buffer_size);
    ofstream ofs(destfilename, ios::binary);
    size_t nRead;

    for (;;) {
        GetLogger()->trace("Copying at most {} bytes from remote to local file...", system_preferred_buffer_size);
        switch (readerPtr->Read(&nRead, buffer.get(), 1, system_preferred_buffer_size)) {
        case 0:
            ofs.write(buffer.get(), (streamsize)nRead);
            continue;
        case -1:
            return -1;
        case -2: // Read at EOF
            break;
        }
        break;
    }

    if (Close(readerPtr->GetHandle())) {
        return -1;
    }
    return 0;
}

int CopyFromLocal(const string &sourcefilename, const string &destfilename) {
    unique_ptr<ServiceRequest> request;
    if (BuildServiceRequest(&request, destfilename)) {
        return -1;
    }
    FileStream *writerPtr;
    if (OpenForWriting(&writerPtr, destfilename)) {
        return -1;
    }
    unique_ptr<char[]> buffer = make_unique<char[]>(system_preferred_buffer_size);
    size_t nRead;
    ifstream ifs(sourcefilename, ios::binary);

    for (;;) {
        GetLogger()->trace("Copying at most {} bytes from local file to remote...", nPreferredBufferSize);
        ifs.read(buffer.get(), nPreferredBufferSize);
        nRead = (size_t)ifs.gcount();
        if (nRead == 0) {
            break;
        }
        int nWriteStatus;
        size_t nWritten;
        if ((nWriteStatus = writerPtr->Write(&nWritten, buffer.get(), 1, nRead))) {
            return nWriteStatus;
        }
    }

    if (Close(writerPtr->GetHandle())) {
        return -1;
    }
    return 0;
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