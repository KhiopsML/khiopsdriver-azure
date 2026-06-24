#include "khiops_driver_azure/core.hpp"
#include <memory>
#include <iomanip>
#include <azure/storage/blobs/block_blob_client.hpp>
#include <khiops_driver_common/logging.hpp>
#include "khiops_driver_common/util.hpp"
#include "khiops_driver_common/globalstate.hpp"
#include "khiops_driver_azure/filestream.hpp"
#include "khiops_driver_azure/auth.hpp"

using namespace std;

using namespace khiops_driver_common;

namespace khiops_driver_azure {

int GetSystemPreferredBufferSize(size_t *result) {
    constexpr size_t DEFAULT_PREFERRED_BUFFER_SIZE = 4ULL * 1024ULL * 1024ULL;
    const string ENVIRONMENT_VARIABLE_NAME = "AZURE_PREFERRED_BUFFER_SIZE";
    static unique_ptr<size_t> system_preferred_buffer_size_memo = nullptr;
    if (system_preferred_buffer_size_memo == nullptr) {
        string environment_variable_preferred_buffer_size = GetEnvVar(ENVIRONMENT_VARIABLE_NAME);
        if (!environment_variable_preferred_buffer_size.empty()) {
            try {
                system_preferred_buffer_size_memo = make_unique<size_t>(stoull(environment_variable_preferred_buffer_size));
            } catch (const invalid_argument &) {
                GetLogger()->warn(
                    "Value {} of environment variable {} is not a valid number. Falling back to default {}...",
                    environment_variable_preferred_buffer_size, ENVIRONMENT_VARIABLE_NAME, DEFAULT_PREFERRED_BUFFER_SIZE
                );
                system_preferred_buffer_size_memo = make_unique<size_t>(DEFAULT_PREFERRED_BUFFER_SIZE);
            } catch (const out_of_range &) {
                GetLogger()->warn(
                    "Value {} of environment variable {} is out of range. Falling back to default {}...",
                    environment_variable_preferred_buffer_size, ENVIRONMENT_VARIABLE_NAME, DEFAULT_PREFERRED_BUFFER_SIZE
                );
                system_preferred_buffer_size_memo = make_unique<size_t>(DEFAULT_PREFERRED_BUFFER_SIZE);
            }
        } else /* environment variable not set */ {
            system_preferred_buffer_size_memo = make_unique<size_t>(DEFAULT_PREFERRED_BUFFER_SIZE);
        }
    }
    *result = *system_preferred_buffer_size_memo;
    return 0;
}

int Remove(const vector<string> &fragment_urls, StorageType storage_type) {
    if (storage_type == BLOB) {
        Azure::Storage::Blobs::DeleteBlobOptions opts;
        opts.DeleteSnapshots = Azure::Storage::Blobs::Models::DeleteSnapshotsOption::IncludeSnapshots;
        for (const auto &url : fragment_urls) {
            Azure::Storage::Blobs::BlobClient client("");
            if (GetBlobClient(&client, url) != 0) return -1;
            try {
                if (!client.Delete(opts).Value.Deleted) {
                    GetLogger()->error("Failed to delete blob {}.", url);
                    return -1;
                }
            } catch (const Azure::Core::RequestFailedException &exc) {
                if (exc.StatusCode == Azure::Core::Http::HttpStatusCode::NotFound) {
                    continue;
                } else {
                    GetLogger()->error("Failed to delete blob {}.", url);
                    return -1;
                }
            }
        }
    } else /* SHARE */ {
        for (const auto &url : fragment_urls) {
            Azure::Storage::Files::Shares::ShareFileClient client("");
            if (GetFileClient(&client, url) != 0) return -1;
            try {
                if (!client.Delete().Value.Deleted) {
                    GetLogger()->error("Failed to delete file {}.", url);
                    return -1;
                }
            } catch (const Azure::Core::RequestFailedException &exc) {
                if (exc.StatusCode == Azure::Core::Http::HttpStatusCode::NotFound) {
                    continue;
                } else {
                    GetLogger()->error("Failed to delete file {}.", url);
                    return -1;
                }
            }
        }
    }
    return 0;
}

int ConcatBlob(const string &dest_url, const vector<string> &source_urls, const vector<string> &source_blob_containers, const vector<string> &source_blobs) {
    try {
        Azure::Storage::Blobs::BlobClient dest_blob_client("");
        if (GetBlobClient(&dest_blob_client, dest_url)) return -1;
        Azure::Storage::Blobs::BlockBlobClient dest_block_blob_client = dest_blob_client.AsBlockBlobClient();
        vector<string> dest_block_ids;
        for (size_t i = 0ULL; i < source_urls.size(); i++) {
            Auth source_auth;
            if (BuildBlobAuth(&source_auth, source_urls[i], source_blob_containers[i], source_blobs[i])) return -1;
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
    } catch (const Azure::Core::RequestFailedException &exc) {
        GetLogger()->error("Failed to upload range from URI. Details of Azure error:");
        GetLogger()->error("  Exception message: {}", exc.what());
        GetLogger()->error("  HTTP response headers:");
        for (const auto &header : exc.RawResponse->GetHeaders()) {
            GetLogger()->error("    Header name: '{}'   Header value: '{}'", header.first, header.second);
        }
        return -1;
    }
    if (Remove(source_urls, BLOB)) return -1;
    return 0;
}

int ConcatFileShare(const string &dest_url, const vector<string> &source_urls, const vector<string> &source_file_shares, const vector<vector<string>> &source_file_paths) {
    try {
        Azure::Core::Http::HttpRange range;
        GetLogger()->trace("Concatenating / Output: {}", dest_url);
        Azure::Storage::Files::Shares::ShareFileClient dest_share_file_client("");
        if (GetFileClient(&dest_share_file_client, dest_url)) return -1;
        unordered_map<const string *, int64_t> source_sizes;
        int64_t total_size = 0LL;
        for (size_t i = 0ULL; i < source_urls.size(); i++) {
            GetLogger()->trace("Concatenating / Input: {}", source_urls[i]);
            Azure::Storage::Files::Shares::ShareFileClient source_share_file_client("");
            if (GetFileClient(&source_share_file_client, source_urls[i])) return -1;
            int64_t source_size = source_share_file_client.GetProperties().Value.FileSize;
            GetLogger()->trace("Concatenating / Size of input: {}", source_size);
            source_sizes[&source_urls[i]] = source_size;
            total_size += source_size;
        }
        dest_share_file_client.Create(total_size);
        GetLogger()->trace("Concatenating / Created destination file of size: {}", total_size);
        int64_t global_offset = 0LL;
        for (size_t i = 0ULL; i < source_urls.size(); i++) {
            int64_t source_size = source_sizes[&source_urls[i]];
            Auth source_auth;
            if (BuildFileShareAuth(&source_auth, source_urls[i], source_file_shares[i], source_file_paths[i])) return -1;
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
    } catch (const Azure::Core::RequestFailedException &exc) {
        GetLogger()->error("Failed to upload range from URI. Details of Azure error:");
        GetLogger()->error("  Exception message: {}", exc.what());
        GetLogger()->error("  HTTP response headers:");
        for (const auto &header : exc.RawResponse->GetHeaders()) {
            GetLogger()->error("    Header name: '{}'   Header value: '{}'", header.first, header.second);
        }
        return -1;
    }
    if (Remove(source_urls, FILE_SHARE)) return -1;
    return 0;
}

}