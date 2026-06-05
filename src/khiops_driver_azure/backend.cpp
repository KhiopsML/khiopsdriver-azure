#include "khiops_driver_common/backend.hpp"
#include "khiops_driver_common/logging.hpp"
#include "khiops_driver_common/util.hpp"
#include "khiops_driver_common/filestream.hpp"
#include "khiops_driver_azure/util.hpp"
#include "khiops_driver_azure/version.hpp"
#include <memory>
#include <algorithm>
#include <iterator>
#include <fstream>
#include <iomanip>
#include <limits>
#include <cstring>
#include <spdlog/spdlog.h>
#include <azure/core/diagnostics/logger.hpp>
#include <azure/storage/common/storage_exception.hpp>
#include <azure/storage/blobs/block_blob_client.hpp>

using namespace std;
using namespace khiops_driver_azure;

namespace khiops_driver_common {

namespace { struct FileWriterUserData {
    // Used only for blob storage
    unique_ptr<vector<string>> block_ids = nullptr;
    // Used only for blob storage
    unique_ptr<Azure::Storage::Blobs::BlobClient> blob_client;
    // Used only for file share storage
    unique_ptr<Azure::Storage::Files::Shares::ShareFileClient> share_file_client;
}; }

spdlog::logger *GetLogger() {
    return GetLogger("azdriver", "AZURE_DRIVER_LOGFILE", "AZURE_DRIVER_LOGLEVEL");
}

void FreeFileReaderFragmentVersion(void *version) {
    delete static_cast<Azure::ETag *>(version);
}

void FreeFileWriterUserData(void *user_data) {
    delete static_cast<FileWriterUserData *>(user_data);
}

int InitializeFileWriterWithWriteMode(FileWriter *file_writer) {
    if (file_writer == nullptr) { GetLogger()->error("Passed null pointer to function {}.", __func__); return -1; }
    FileWriterUserData *user_data = new FileWriterUserData();
    file_writer->user_data.reset(static_cast<void *>(user_data));
    unique_ptr<ServiceRequest> request;
    if (BuildServiceRequest(&request, file_writer->url) != 0) return -1;
    if (request->storage_type == BLOB) {
        user_data->block_ids = make_unique<vector<string>>();
        user_data->blob_client = make_unique<Azure::Storage::Blobs::BlobClient>("");
        if (GetBlobClient(user_data->blob_client.get(), *request) != 0) return -1;
    } else if (request->storage_type == SHARE) {
        user_data->share_file_client = make_unique<Azure::Storage::Files::Shares::ShareFileClient>("");
        if (GetFileClient(user_data->share_file_client.get(), *request) != 0) return -1;
        user_data->share_file_client->Create(0LL);
    }
    return 0;
}

int InitializeFileWriteWithAppendMode(FileWriter *file_writer) {
    if (file_writer == nullptr) { GetLogger()->error("Passed null pointer to function {}.", __func__); return -1; }
    FileWriterUserData *user_data = new FileWriterUserData();
    file_writer->user_data.reset(static_cast<void *>(user_data));
    unique_ptr<ServiceRequest> request;
    if (BuildServiceRequest(&request, file_writer->url) != 0) return -1;
    if (request->storage_type == BLOB) {
        user_data->block_ids = make_unique<vector<string>>();
        user_data->blob_client = make_unique<Azure::Storage::Blobs::BlobClient>("");
        if (GetBlobClient(user_data->blob_client.get(), *request) != 0) return -1;
        try {
            auto block_list_request_response = user_data->blob_client->AsBlockBlobClient().GetBlockList();
            vector<Azure::Storage::Blobs::Models::BlobBlock> blocks = block_list_request_response.Value.CommittedBlocks;
            transform(blocks.begin(), blocks.end(), back_inserter(*user_data->block_ids), [](const auto &block) { return block.Name; });
        } catch (const Azure::Storage::StorageException &) {}
    } else if (request->storage_type == SHARE) {
        user_data->share_file_client = make_unique<Azure::Storage::Files::Shares::ShareFileClient>("");
        if (GetFileClient(user_data->share_file_client.get(), *request) != 0) return -1;
    }
    return 0;
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
    if (result == nullptr || stopped_on_termchar == nullptr) {
        GetLogger()->error("Null pointer passed to function {}.", __func__);
        return -1;
    }
    if (termchar == nullptr) {
        GetLogger()->debug("Reading a maximum of {} bytes from fragment at URL {} starting at offset {} (no terminator character specified)...", maxlength, url, offset);
    } else {
        GetLogger()->debug("Reading a maximum of {} bytes from fragment at URL {} starting at offset {} (can also end if terminator character '{}' is found)...", maxlength, url, offset, *termchar);
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
                    } else if (exc.StatusCode == Azure::Core::Http::HttpStatusCode::RangeNotSatisfiable) {
                        GetLogger()->error("Cannot read after end of file.");
                        return -1;
                    } else {
                        throw;
                    }
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

int FCloseReader(const FileReader &) {
    // Nothing to do.
    return 0;
}

int FCloseWriter(const FileWriter &stream) {
    if (FFlush(stream) == 0) return 0;
    else return -1;
}

// int FRead(size_t *result, void *ptr, FileReader *file_reader, size_t size, size_t count) {
//     size_t nlefttoread = ntotaltoread, ntotalread = 0ULL, ntoread, nread, offset_inside_first_fragment_to_read, fragment_remote_offset, fragment_index;
//     string globalread, read;
//     bool stopped_on_term_char, first_fragment_to_read = true;

//     GetLogger()->debug("FReading {} bytes from file of total size {} starting at position {}...", ntotaltoread, file_reader->total_size, file_reader->current_position);

//     // if (ntotaltoread == 0) { *result = 0ULL; return 0; }
//     // if (file_reader->current_position == file_reader->total_size) { GetLogger()->error("Cannot read after end of file."); return -1; }
//     if (FragmentIndexOfUserOffset(&fragment_index, *file_reader, file_reader->current_position) != 0) return -1;
//     while (nlefttoread != 0ULL && file_reader->current_position != file_reader->total_size) {
//         const FileReader::Fragment &fragment = file_reader->fragments[fragment_index];
//         if (first_fragment_to_read) {
//             offset_inside_first_fragment_to_read = file_reader->current_position - fragment.user_offset;
//             fragment_remote_offset = (fragment_index == 0ULL ? 0ULL : file_reader->header_length) + offset_inside_first_fragment_to_read;
//             ntoread = min(nlefttoread, fragment.content_size - offset_inside_first_fragment_to_read);
//         } else {
//             fragment_remote_offset = file_reader->header_length;
//             ntoread = min(nlefttoread, fragment.content_size);
//         }
//         if (ReadFragment(&read, &stopped_on_term_char, fragment.url, fragment.version.get(), fragment_remote_offset, ntoread) != 0) {
//             return -1;
//         }
//         nread = read.size();
//         if (nread != ntoread) { GetLogger()->error("Failed to read."); return -1; }
//         ntotalread += nread;
//         globalread.append(read);
//         nlefttoread -= nread;
//         fragment_index++;
//         first_fragment_to_read = false;
//         file_reader->current_position += nread;
//     }
//     *result = ntotalread;
//     memcpy(ptr, globalread.data(), ntotalread);
//     return 0;
// }

int FWrite(size_t *result, FileWriter *file_writer, const void *ptr, size_t size, size_t count) {
    if (result == nullptr || file_writer == nullptr || ptr == nullptr) { GetLogger()->error("Null pointer passed to function {}.", __func__); return -1; }
    if (size != 0 && count > numeric_limits<size_t>::max() / size) { GetLogger()->error("Asked number of bytes to write overflows."); return -1; }

    size_t ntotaltowrite = size * count;
    unique_ptr<ServiceRequest> request;
    if (BuildServiceRequest(&request, file_writer->url) != 0) return -1;
    FileWriterUserData *user_data = static_cast<FileWriterUserData *>(file_writer->user_data.get());
    if (request->storage_type == BLOB) {
        Azure::Storage::Blobs::BlockBlobClient bbclient = user_data->blob_client->AsBlockBlobClient();
        ostringstream oss;
        oss << setfill('0') << setw(64) << user_data->block_ids->size();
        string block_id_in_base10 = oss.str();
        vector<uint8_t> block_id_in_base_10_as_vec(block_id_in_base10.begin(), block_id_in_base10.end());
        string block_id_in_base64 = Azure::Core::Convert::Base64Encode(block_id_in_base_10_as_vec);
        Azure::Core::IO::MemoryBodyStream body_stream(static_cast<const uint8_t *>(ptr), ntotaltowrite);
        bbclient.StageBlock(block_id_in_base64, body_stream);
        user_data->block_ids->push_back(block_id_in_base64);
    } else if (request->storage_type == SHARE) {
        Azure::Storage::Files::Shares::Models::FileHttpHeaders http_headers;
        Azure::Storage::Files::Shares::Models::FileSmbProperties smb_properties;
        Azure::Storage::Files::Shares::SetFilePropertiesOptions opts;
        Azure::Core::IO::MemoryBodyStream body_stream(static_cast<const uint8_t *>(ptr), ntotaltowrite);
        opts.Size = file_writer->current_position + ntotaltowrite;
        user_data->share_file_client->SetProperties(http_headers, smb_properties, opts);
        user_data->share_file_client->UploadRange(static_cast<int64_t>(file_writer->current_position), body_stream);
        file_writer->current_position += ntotaltowrite;
    }
    *result = ntotaltowrite;
    return 0;
}

int FFlush(const FileWriter &file_writer) {
    unique_ptr<ServiceRequest> request;
    if (BuildServiceRequest(&request, file_writer.url) != 0) return -1;
    FileWriterUserData *user_data = static_cast<FileWriterUserData *>(file_writer.user_data.get());
    if (request->storage_type == BLOB) {
        user_data->blob_client->AsBlockBlobClient().CommitBlockList(*user_data->block_ids);
    } else if (request->storage_type == SHARE) {
        user_data->share_file_client->ForceCloseAllHandles();
    }
    return 0;
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

int DiskFreeSpace(size_t *result, const string &) {
    *result = 5ULL * 1024ULL * 1024ULL * 1024ULL * 1024ULL;
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