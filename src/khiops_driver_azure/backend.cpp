#include "khiops_driver_common/backend.hpp"
#include "khiops_driver_azure/auth.hpp"
#include "khiops_driver_common/logging.hpp"
#include "khiops_driver_common/util.hpp"
#include "khiops_driver_common/filestream.hpp"
#include "khiops_driver_azure/util.hpp"
#include "khiops_driver_azure/version.hpp"
#include "khiops_driver_azure/connstr.hpp"
#include <memory>
#include <algorithm>
#include <iterator>
#include <fstream>
#include <iomanip>
#include <limits>
#include <unordered_map>
#include <cstring>
#include <spdlog/spdlog.h>
#include <azure/core/diagnostics/logger.hpp>
#include <azure/storage/common/storage_exception.hpp>
#include <azure/storage/blobs/block_blob_client.hpp>
#include <azure/identity.hpp>

using namespace std;
using namespace khiops_driver_azure;

namespace khiops_driver_common {

int CreateRemoteObjectRequestUserData(CustomVoidUniquePtr *result, const string &url) {
    if (result == nullptr) { GetLogger()->error("Passed null pointer to function {}.", __func__); return -1; }

    // Perform initial URL parsing using Azure SDK.
    Azure::Core::Url azure_url;
    try {
        azure_url = Azure::Core::Url(url);
    } catch (const exception &) {
        GetLogger()->error("Caught an exception while performing basic URL parsing: URL {} is invalid.", url);
        return -1;
    }
    
    // Determine whether storage service is emulated or not.
    bool is_emulated_storage = IsEmulatedStorage();

    // Get type of storage service.
    StorageType storage_type;
    if (StorageTypeOfUrl(&storage_type, azure_url, is_emulated_storage) != 0) return -1;

    // Split path of remote object.
    ObjectPath object_path;
    if (ObjectPathOfUrl(&object_path, azure_url, is_emulated_storage, storage_type) != 0) return -1;

    // Get service URL.
    ostringstream oss;
    oss << azure_url.GetScheme() << "://" << azure_url.GetHost();
    if (azure_url.GetPort() > 0) oss << ":" << azure_url.GetPort();
    if (is_emulated_storage) oss << "/" << *object_path.emulated_account_name;
    string service_url = oss.str();

    // Parse connection string.
    string connection_string_as_string = GetEnvVar("AZURE_STORAGE_CONNECTION_STRING");
    bool is_using_connection_string = !connection_string_as_string.empty();
    if (is_emulated_storage && !is_using_connection_string) {
        GetLogger()->error("Undefined or empty environment variable: AZURE_STORAGE_CONNECTION_STRING.");
        return -1;
    }
    ConnectionString connection_string;
    if (ConnectionString::ParseConnectionString(&connection_string, connection_string_as_string, is_emulated_storage) != 0) return -1;
    if (connection_string.CheckAgainstUrl(azure_url, storage_type) != 0) return -1;

    // Credentials
    shared_ptr<Azure::Storage::StorageSharedKeyCredential> connection_string_credential;
    shared_ptr<Azure::Identity::ChainedTokenCredential> no_connection_string_credential;
    if (is_using_connection_string) {
        connection_string_credential = make_shared<Azure::Storage::StorageSharedKeyCredential>(connection_string.sAccountName, connection_string.sAccountKey);
    } else {
        no_connection_string_credential = make_shared<Azure::Identity::ChainedTokenCredential>(
            Azure::Identity::ChainedTokenCredential::Sources {
                std::make_shared<Azure::Identity::EnvironmentCredential>(),  // for Client ID + Client Secret or Certificate environment variables
                std::make_shared<Azure::Identity::WorkloadIdentityCredential>(),
                std::make_shared<Azure::Identity::ManagedIdentityCredential>(),
                std::make_shared<Azure::Identity::AzureCliCredential>()
            }
        );
    }

    RemoteObjectRequestUserData *request_user_data = new RemoteObjectRequestUserData();
    request_user_data->service_url = service_url;
    request_user_data->is_emulated_storage = is_emulated_storage;
    request_user_data->storage_type = storage_type;
    request_user_data->object_path = std::move(object_path);
    request_user_data->is_using_connection_string = is_using_connection_string;
    request_user_data->connection_string_credential = connection_string_credential;
    request_user_data->no_connection_string_credential = no_connection_string_credential;
    
    GetLogger()->debug("Remote object request user data:");
    GetLogger()->debug("  service URL: {}", request_user_data->service_url);
    GetLogger()->debug("  storage emulated: {}", request_user_data->is_emulated_storage ? "yes" : "no");
    GetLogger()->debug("  storage type: {}", request_user_data->storage_type == BLOB ? "blob" : "file share");
    GetLogger()->debug("  object path: {}", ObjectPathToString(request_user_data->object_path));
    GetLogger()->debug("  using connection string: {}", request_user_data->is_using_connection_string ? "yes" : "no");

    *result = CustomVoidUniquePtr(request_user_data, &FreeRemoteObjectRequestUserData);
    return 0;
}

void FreeRemoteObjectRequestUserData(void *user_data) {
    delete static_cast<RemoteObjectRequestUserData *>(user_data);
}

spdlog::logger *GetLogger() {
    return GetLogger("azdriver", "AZURE_DRIVER_LOGFILE", "AZURE_DRIVER_LOGLEVEL");
}

void FreeFileReaderFragmentVersion(void *version) {
    delete static_cast<Azure::ETag *>(version);
}

int CreateFileWriterUserDataInWriteMode(CustomVoidUniquePtr *result, const RemoteObjectRequest &request) {
    if (result == nullptr) { GetLogger()->error("Passed null pointer to function {}.", __func__); return -1; }
    FileWriterUserData *writer_user_data = new FileWriterUserData();
    const RemoteObjectRequestUserData *request_user_data = GetUserData(request);
    if (request_user_data->storage_type == BLOB) {
        writer_user_data->block_ids = make_unique<vector<string>>();
        writer_user_data->blob_client = make_unique<Azure::Storage::Blobs::BlobClient>("");
        if (GetBlobClient(writer_user_data->blob_client.get(), request) != 0) return -1;
    } else if (request_user_data->storage_type == FILE_SHARE) {
        writer_user_data->share_file_client = make_unique<Azure::Storage::Files::Shares::ShareFileClient>("");
        if (GetFileClient(writer_user_data->share_file_client.get(), request) != 0) return -1;
        writer_user_data->share_file_client->Create(0LL);
    }
    *result = CustomVoidUniquePtr(writer_user_data, &FreeFileWriterUserData);
    return 0;
}

int CreateFileWriterUserDataInAppendMode(CustomVoidUniquePtr *result, const RemoteObjectRequest &request) {
    if (result == nullptr) { GetLogger()->error("Passed null pointer to function {}.", __func__); return -1; }
    FileWriterUserData *writer_user_data = new FileWriterUserData();
    const RemoteObjectRequestUserData *request_user_data = GetUserData(request);
    if (request_user_data->storage_type == BLOB) {
        writer_user_data->block_ids = make_unique<vector<string>>();
        writer_user_data->blob_client = make_unique<Azure::Storage::Blobs::BlobClient>("");
        if (GetBlobClient(writer_user_data->blob_client.get(), request) != 0) return -1;
        try {
            auto block_list_request_response = writer_user_data->blob_client->AsBlockBlobClient().GetBlockList();
            vector<Azure::Storage::Blobs::Models::BlobBlock> blocks = block_list_request_response.Value.CommittedBlocks;
            transform(blocks.begin(), blocks.end(), back_inserter(*writer_user_data->block_ids), [](const auto &block) { return block.Name; });
        } catch (const Azure::Storage::StorageException &) {}
    } else if (request_user_data->storage_type == FILE_SHARE) {
        writer_user_data->share_file_client = make_unique<Azure::Storage::Files::Shares::ShareFileClient>("");
        if (GetFileClient(writer_user_data->share_file_client.get(), request) != 0) return -1;
    }
    *result = CustomVoidUniquePtr(writer_user_data, &FreeFileWriterUserData);
    return 0;
}

void FreeFileWriterUserData(void *user_data) {
    delete static_cast<FileWriterUserData *>(user_data);
}

int ListFragments(vector<string> *result, const RemoteObjectRequest &request) {
    *result = std::move(ListBlobsOrFiles(request));
    return 0;
}

int GetFragmentSizeAndVersion(size_t *size_result, void **version_result, const RemoteObjectRequest &request) {
    if (GetUserData(request)->storage_type == BLOB) {
        Azure::Storage::Blobs::BlobClient client("");
        if (GetBlobClient(&client, request) == 0) {
            auto blob_properties = std::move(client.GetProperties().Value);
            *size_result = static_cast<size_t>(blob_properties.BlobSize);
            *version_result = static_cast<void *>(new Azure::ETag(blob_properties.ETag.ToString()));
            return 0;
        }
    } else /* SHARE */ {
        Azure::Storage::Files::Shares::ShareFileClient client("");
        if (GetFileClient(&client, request) == 0) {
            auto file_properties = std::move(client.GetProperties().Value);
            *size_result = static_cast<size_t>(file_properties.FileSize);
            *version_result = static_cast<void *>(new Azure::ETag(file_properties.ETag.ToString()));
            return 0;
        }
    }
}

static int ReadFragment(string *result, bool *stopped_on_termchar, const RemoteObjectRequest &request, void *version, size_t offset, size_t maxlength, const char *termchar) {
    if (result == nullptr || stopped_on_termchar == nullptr) {
        GetLogger()->error("Null pointer passed to function {}.", __func__);
        return -1;
    }
    if (termchar == nullptr) {
        GetLogger()->debug("Reading a maximum of {} bytes from fragment at URL {} starting at offset {} (no terminator character specified)...", maxlength, request.url, offset);
    } else {
        GetLogger()->debug("Reading a maximum of {} bytes from fragment at URL {} starting at offset {} (can also end if terminator character '{}' is found)...", maxlength, request.url, offset, *termchar);
    }
    string content_read = "";
    unique_ptr<Azure::Core::IO::BodyStream> body_stream;
    size_t buffer_size;
    Azure::ETag previousETag = *static_cast<Azure::ETag *>(version);
    size_t number_of_bytes_to_read = maxlength;
    size_t number_of_bytes_read = 0ULL;
    uint8_t *termchar_pos;
    Azure::Core::Http::HttpRange range;
    range.Offset = static_cast<int64_t>(offset);
    if (GetSystemPreferredBufferSize(&buffer_size) == 0) {
        vector<uint8_t> buffer(buffer_size);
        uint8_t *buffer_start = buffer.data();
        while (number_of_bytes_to_read > 0ULL) {
            range.Offset += number_of_bytes_read;
            range.Length = static_cast<int64_t>(min(number_of_bytes_to_read, buffer_size));
            try {
                if (GetUserData(request)->storage_type == BLOB) {
                    Azure::Storage::Blobs::BlobAccessConditions access_conditions;
                    access_conditions.IfMatch = previousETag;
                    Azure::Storage::Blobs::DownloadBlobOptions opts;
                    opts.Range = range;
                    opts.AccessConditions = access_conditions;
                    Azure::Storage::Blobs::BlobClient client("");
                    if (GetBlobClient(&client, request) != 0) {
                        return -1;
                    }
                    body_stream = std::move(client.Download(opts).Value.BodyStream);
                } else /* SHARE */ {
                    Azure::Storage::Files::Shares::DownloadFileOptions opts;
                    opts.Range = range;
                    Azure::Storage::Files::Shares::ShareFileClient client("");
                    if (GetFileClient(&client, request) != 0) {
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
    return -1;
}

int ReadFragment(string *result, bool *stopped_on_termchar, const RemoteObjectRequest &request, void *version, size_t offset, size_t maxlength) {
    return ReadFragment(result, stopped_on_termchar, request, version, offset, maxlength, nullptr);
}

int ReadFragment(string *result, bool *stopped_on_termchar, const RemoteObjectRequest &request, void *version, size_t offset, size_t maxlength, char termchar) {
    return ReadFragment(result, stopped_on_termchar, request, version, offset, maxlength, &termchar);
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
        string environment_variable_preferred_buffer_size = GetEnvVar(ENVIRONMENT_VARIABLE_NAME);
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

int FileExists(bool *result, const RemoteObjectRequest &request) {
    *result = !ListBlobsOrFiles(request).empty();
    return 0;
}

int DirExists(bool *result, const RemoteObjectRequest &request) {
    if (GetUserData(request)->storage_type == BLOB) {
        *result = true;  // there is no such concept as a directory when dealing with blob services
    } else /* SHARE */ {
        *result = !ListDirs(request).empty();
    }
    return 0;
}

int GetFileSize(size_t *result, const RemoteObjectRequest &request) {
    vector<string> objects;
    if (ListBlobsOrFilesCheckNotEmpty(&objects, request) == 0) {
        FileReader file_reader;
        PopulateFileReader(&file_reader, request);
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
    } else if (request->storage_type == FILE_SHARE) {
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
    FileWriterUserData *user_data = static_cast<FileWriterUserData *>(file_writer.user_data.get());
    if (request->storage_type == BLOB) {
        user_data->blob_client->AsBlockBlobClient().CommitBlockList(*user_data->block_ids);
    } else if (request->storage_type == FILE_SHARE) {
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
    return 0;
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
    vector<unique_ptr<ServiceRequest>> inputs(sourcefilenames.size());
    for (size_t i = 0ULL; i < sourcefilenames.size(); i++) {
        if (BuildServiceRequest(&inputs[i], sourcefilenames[i]) != 0) return -1;
    }
    unique_ptr<ServiceRequest> output;
    if (BuildServiceRequest(&output, destfilename) != 0) return -1;
    for (const unique_ptr<ServiceRequest> &input : inputs) {
        if (input->storage_type != output->storage_type) {
            GetLogger()->error("Input storage type (blob/file) does not match output storage type.");
            return -1;
        }
    }
    try {
        if (output->storage_type == BLOB) {
            Azure::Storage::Blobs::BlobClient dest_blob_client("");
            if (GetBlobClient(&dest_blob_client, *output) != 0) return -1;
            Azure::Storage::Blobs::BlockBlobClient dest_block_blob_client = dest_blob_client.AsBlockBlobClient();
            vector<string> dest_block_ids;
            for (const unique_ptr<ServiceRequest> &input : inputs) {
                Auth source_auth;
                if (BuildAuth(&source_auth, *input) != 0) return -1;
                Azure::Storage::Blobs::StageBlockFromUriOptions opts;
                if (source_auth.HasHeader()) opts.SourceAuthorization = source_auth.sAuthHeader;
                ostringstream oss;
                oss << setfill('0') << setw(64) << dest_block_ids.size();
                string block_id_in_base10 = oss.str();
                vector<uint8_t> block_id_in_base10_vec(block_id_in_base10.begin(), block_id_in_base10.end());
                string block_id_in_base64 = Azure::Core::Convert::Base64Encode(block_id_in_base10_vec);
                dest_block_ids.push_back(block_id_in_base64);
                dest_block_blob_client.StageBlockFromUri(block_id_in_base64, source_auth.sUriAuth, opts);
            }
            dest_block_blob_client.CommitBlockList(dest_block_ids);
        } else /* SHARE */ {
            Azure::Core::Http::HttpRange range;
            GetLogger()->trace("Concatenating / Output: {}", output->azure_url.GetAbsoluteUrl());
            Azure::Storage::Files::Shares::ShareFileClient dest_share_file_client("");
            if (GetFileClient(&dest_share_file_client, *output) != 0) return -1;
            unordered_map<const ServiceRequest *, int64_t> source_sizes;
            int64_t total_size = 0LL;
            for (const unique_ptr<ServiceRequest> &input : inputs) {
                GetLogger()->trace("Concatenating / Input: {}", input->azure_url.GetAbsoluteUrl());
                Azure::Storage::Files::Shares::ShareFileClient source_share_file_client("");
                if (GetFileClient(&source_share_file_client, *input) != 0) return -1;
                int64_t source_size = source_share_file_client.GetProperties().Value.FileSize;
                GetLogger()->trace("Concatenating / Size of input: {}", source_size);
                source_sizes[input.get()] = source_size;
                total_size += source_size;
            }
            dest_share_file_client.Create(total_size);
            GetLogger()->trace("Concatenating / Created destination file of size: {}", total_size);
            int64_t global_offset = 0LL;
            for (const unique_ptr<ServiceRequest> &input : inputs) {
                int64_t source_size = source_sizes[input.get()];
                Auth source_auth;
                if (BuildAuth(&source_auth, *input) != 0) return -1;
                Azure::Storage::Files::Shares::UploadFileRangeFromUriOptions opts;
                if (source_auth.HasHeader()) opts.SourceAuthorization = source_auth.sAuthHeader;
                // See size limitation of source range: header x-ms-source-range at https://learn.microsoft.com/en-us/rest/api/storageservices/put-range-from-url.
                constexpr int64_t MAX_SOURCE_SIZE = 4LL * 1024LL * 1024LL;
                for (int64_t offset_in_source = 0LL; offset_in_source < source_size; offset_in_source += MAX_SOURCE_SIZE) {
                    int64_t to_upload = min(source_size - offset_in_source, MAX_SOURCE_SIZE);
                    range = Azure::Core::Http::HttpRange{offset_in_source, to_upload};
                    dest_share_file_client.UploadRangeFromUri(global_offset + offset_in_source, source_auth.sUriAuth, range, opts);
                }
                global_offset += source_size;
            }
        }
    } catch (const Azure::Core::RequestFailedException &exc) {
        GetLogger()->error("Failed to upload range from URI. Details of Azure error:");
        GetLogger()->error("  Exception message: {}", exc.what());
        GetLogger()->error("  HTTP response headers:");
        for (const auto &header : exc.RawResponse->GetHeaders()) {
            GetLogger()->error("    Header name: '{}'   Header value: '{}'", header.first, header.second);
        }
        return -1;
    }

    for (const string &sourcefilename : sourcefilenames) {
        if (Remove(sourcefilename) != 0) return -1;
    }

    return 0;
}

int ComposeMultifile(const string &sDestFilePathName, const vector<string> &sSourceFilePathNames) {
    GetLogger()->error("Not implemented!");
    return -1;
}

}