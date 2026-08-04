#include "khiops_driver_azure/filestream.hpp"
#include "khiops_driver_azure/util.hpp"
#include "khiops_driver_common/logging.hpp"
#include "khiops_driver_common/globalstate.hpp"
#include "khiops_driver_common/util.hpp"
#include "khiops_driver_common/checks.hpp"
#include "khiops_driver_common/stringify.hpp"
#include <algorithm>
#include <iomanip>
#include <memory>
#include <azure/storage/common/storage_exception.hpp>
#include <azure/storage/blobs/block_blob_client.hpp>

using namespace std;

using namespace khiops_driver_common;

namespace khiops_driver_azure {

int PopulateFileReader(FileReader *file_reader, StorageType storage_type, const vector<string> &fragment_urls) {
    if (CheckNotNull(file_reader, STRINGIFY(file_reader), __func__)) return -1;

    // Number of fragments to pick at each end for header detection.
    const size_t NUMBER_OF_FRAGMENTS_TO_PICK_AT_EACH_END = 5;
    // Number of fragments to pick randomly for header detection. This is only a maximum since priority is given to the "pick 'n' at each end" rule.
    const size_t MAX_NUMBER_OF_FRAGMENTS_TO_PICK_RANDOMLY = 10;
    // The maximal size of a header is defined to 8 MiB. If there is a longer header, it is not considered to be a header.
    constexpr size_t MAX_HEADER_LENGTH = 8ULL * 1024ULL * 1024ULL;

    size_t number_of_fragments_picked_randomly = 0ULL;
    
    //     ** A NOTE ABOUT REPEATED HEADERS **
    //
    // A repeated header is a header that is repeated at the beginning of every fragment.
    // From the user point of view, the header is a part of the content of the first fragment only.
    
    // Before any readings are performed, we do not know it there is a repeated header, so there MAY BE one.
    bool there_may_be_a_header = true;

    string possible_header;
    string header_just_read;
    size_t possible_header_length = MAX_HEADER_LENGTH;

    const size_t total_number_of_fragments = fragment_urls.size();
    vector<size_t> fragment_sizes(total_number_of_fragments);
    vector<Azure::ETag> fragment_versions(total_number_of_fragments);
    
    for (size_t fragment_index = 0ULL; fragment_index < total_number_of_fragments; fragment_index++) {
        if (storage_type == BLOB) {
            Azure::Storage::Blobs::BlobClient client("");
            if (GetBlobClient(&client, fragment_urls[fragment_index]) != 0) return -1;
            auto blob_properties = std::move(client.GetProperties().Value);
            fragment_sizes[fragment_index] = static_cast<size_t>(blob_properties.BlobSize);
            fragment_versions[fragment_index] = blob_properties.ETag;
        } else /* SHARE */ {
            Azure::Storage::Files::Shares::ShareFileClient client("");
            if (GetFileClient(&client, fragment_urls[fragment_index]) != 0) return -1;
            auto file_properties = std::move(client.GetProperties().Value);
            fragment_sizes[fragment_index] = static_cast<size_t>(file_properties.FileSize);
            fragment_versions[fragment_index] = file_properties.ETag;
        }

        if (fragment_index == 0ULL) {
            possible_header_length = min(fragment_sizes[fragment_index], MAX_HEADER_LENGTH);
        }

        if (there_may_be_a_header) {
            if (fragment_index > 0ULL && fragment_sizes[fragment_index] < possible_header_length) {
                // A header has previously been detected but it is too big to fit inside the current fragment.
                there_may_be_a_header = false;
            }

            // Determine if we need to fetch the header of the current fragment.
            bool should_read_header = false;
            if (fragment_index < NUMBER_OF_FRAGMENTS_TO_PICK_AT_EACH_END || total_number_of_fragments <= fragment_index + NUMBER_OF_FRAGMENTS_TO_PICK_AT_EACH_END) {
                should_read_header = true;
            } else if (number_of_fragments_picked_randomly < MAX_NUMBER_OF_FRAGMENTS_TO_PICK_RANDOMLY && RandomBool()) {
                should_read_header = true;
                number_of_fragments_picked_randomly++;
            }

            if (should_read_header) {
                // Read the header.
                bool stopped_on_termchar;
                if (ReadFragment(&header_just_read, &stopped_on_termchar, storage_type, fragment_urls[fragment_index], fragment_versions[fragment_index], 0ULL, possible_header_length, '\n')) {
                    // Failed to read the header.
                    return -1;
                }

                if (!stopped_on_termchar || header_just_read.empty() || (fragment_index > 0ULL && header_just_read != possible_header)) {
                    there_may_be_a_header = false;
                } else if (fragment_index == 0ULL) {
                    possible_header_length = header_just_read.size();
                    possible_header = header_just_read;
                }
            }
        }
    }

    // From now on, we know if there is a repeated header or not, and if there is one we know its content and, more importantly, its size.
    bool there_is_a_header = there_may_be_a_header;
    size_t header_length = there_is_a_header ? possible_header_length : 0ULL;
    
    file_reader->total_size = 0ULL;
    file_reader->header_length = header_length;
    file_reader->current_position = 0ULL;
    file_reader->storage_type = storage_type;

    size_t current_user_offset = 0ULL;
    for (size_t fragment_index = 0ULL; fragment_index < total_number_of_fragments; fragment_index++) {
        // Only the first fragment will include the header in its content.
        size_t fragment_content_size = there_is_a_header && fragment_index > 0ULL ? fragment_sizes[fragment_index] - header_length : fragment_sizes[fragment_index];

        // Create the fragment object and add it into the file reader's vector of fragments.
        FileReader::Fragment fragment;
        fragment.url = fragment_urls[fragment_index];
        fragment.user_offset = current_user_offset;
        fragment.content_size = fragment_content_size;
        fragment.version = fragment_versions[fragment_index];
        file_reader->fragments.push_back(std::move(fragment));

        // Update the total size of the file.
        file_reader->total_size += fragment_content_size;

        // Update the current user offset.
        current_user_offset += fragment_content_size;
    }

    return 0;
}

int FragmentIndexOfUserOffset(size_t *result, const FileReader &file_reader, size_t user_offset) {
    if (file_reader.fragments.empty()) { GetLogger()->error("No fragments found."); return -1; }
    if (file_reader.fragments.front().user_offset != 0ULL) { GetLogger()->error("User offset of first fragment is not zero."); return -1; }
    *result = static_cast<size_t>(
        find_if(
            file_reader.fragments.begin(), file_reader.fragments.end(),
            [user_offset](const FileReader::Fragment &fragment) { return user_offset < fragment.user_offset; }
        )
        - 1
        - file_reader.fragments.begin()
    );
    return 0;
}

int FRead(size_t *result, void *ptr, FileReader *file_reader, size_t size, size_t count) {
    if (CheckNotNull(result, STRINGIFY(result), __func__)) return -1;
    if (CheckNotNull(ptr, STRINGIFY(ptr), __func__)) return -1;
    if (CheckNotNull(file_reader, STRINGIFY(file_reader), __func__)) return -1;

    const size_t ntotaltoread = size * count;
    size_t nlefttoread = ntotaltoread, ntotalread = 0ULL, ntoread, nread;
    size_t offset_inside_first_fragment_to_read, fragment_remote_offset;
    size_t absolute_fragment_index, relative_fragment_index;
    uint8_t *output_buffer = static_cast<uint8_t *>(ptr);
    
    GetLogger()->debug("Reading starting position: {}  |  Total number of bytes to read: {}  |  Total file size: {}.", file_reader->current_position, ntotaltoread, file_reader->total_size);
    
    if (nlefttoread == 0ULL) {
        GetLogger()->trace("0 byte to read => fast-exit.");
        *result = 0ULL;
        return 0;
    }

    if (file_reader->current_position == file_reader->total_size) {
        GetLogger()->error("Cannot read after end of file.");
        return -1;
    }

    if (nlefttoread > file_reader->total_size - file_reader->current_position) {
        GetLogger()->debug("Number of bytes to read exceeds number of remaining bytes to read from file => limiting to read rest of file.");
        nlefttoread = file_reader->total_size - file_reader->current_position;
    }

    if (FragmentIndexOfUserOffset(&absolute_fragment_index, *file_reader, file_reader->current_position) != 0) return -1;
    GetLogger()->debug("Selected fragment #{} (0-based index) as the fragment to start reading from (file contains {} fragments).", absolute_fragment_index, file_reader->fragments.size());

    for (relative_fragment_index = 0ULL; nlefttoread > 0ULL ; relative_fragment_index++, absolute_fragment_index++) {
        const FileReader::Fragment &fragment = file_reader->fragments[absolute_fragment_index];
        if (relative_fragment_index == 0ULL) {
            offset_inside_first_fragment_to_read = file_reader->current_position - fragment.user_offset;
            fragment_remote_offset = (absolute_fragment_index == 0ULL ? 0ULL : file_reader->header_length) + offset_inside_first_fragment_to_read;
            ntoread = min(nlefttoread, fragment.content_size - offset_inside_first_fragment_to_read);
        } else {
            fragment_remote_offset = file_reader->header_length;
            ntoread = min(nlefttoread, fragment.content_size);
        }
        if (ReadFragmentToBuffer(&nread, file_reader->storage_type, fragment.url, fragment.version, fragment_remote_offset, ntoread, output_buffer + ntotalread) != 0) {
            GetLogger()->error("Failed to read.");
            return -1;
        }
        GetLogger()->trace("File fragment #{} (absolute #{}): read {} bytes.", relative_fragment_index, absolute_fragment_index, nread);
        if (nread != ntoread) { GetLogger()->error("Number of bytes read does not match number of bytes to read."); return -1; }
        ntotalread += nread;
        nlefttoread -= nread;
        file_reader->current_position += nread;
    }

    *result = ntotalread;
    return 0;
}

int FSeek(FileReader *file_reader, long long int offset, int whence) {
    if (CheckNotNull(file_reader, STRINGIFY(file_reader), __func__)) return -1;
    bool seek_out_of_range = false;
    if (whence == ios::beg) {
        if (offset < 0LL || static_cast<size_t>(offset) > file_reader->total_size) {
            seek_out_of_range = true;
        } else {
            file_reader->current_position = static_cast<size_t>(offset);
        }
    } else if (whence == ios::cur) {
        if (offset < 0LL) {
            size_t positive_offset = static_cast<size_t>(-(offset + 1LL)) + 1ULL;
            if (positive_offset > file_reader->current_position) {
                seek_out_of_range = true;
            } else {
                file_reader->current_position -= positive_offset;
            }
        } else {
            if (static_cast<size_t>(offset) > file_reader->total_size - file_reader->current_position) {
                seek_out_of_range = true;
            } else {
                file_reader->current_position += static_cast<size_t>(offset);
            }
        }
    } else if (whence == ios::end) {
        if (offset < 0LL) {
            size_t positive_offset = static_cast<size_t>(-(offset + 1LL)) + 1ULL;
            if (positive_offset > file_reader->total_size) {
                seek_out_of_range = true;
            } else {
                file_reader->current_position = file_reader->total_size - positive_offset;
            }
        } else if (offset == 0LL) {
            file_reader->current_position = file_reader->total_size;
        }
    }
    if (seek_out_of_range) {
        GetLogger()->error("Seeking out of file's range.");
        return -1;
    }
    return 0;
}

int FClose(FileReader *file_reader) {
    if (CheckNotNull(file_reader, STRINGIFY(file_reader), __func__)) return -1;
    if (UnregisterFileStream(&GetState()->open_file_streams, static_cast<void *>(file_reader))) return -1;
    delete file_reader;
    return 0;
}

int FWrite(size_t *result, FileWriter *file_writer, const void *ptr, size_t size, size_t count) {
    if (CheckNotNull(result, STRINGIFY(result), __func__)) return -1;
    if (CheckNotNull(file_writer, STRINGIFY(file_writer), __func__)) return -1;
    if (CheckNotNull(ptr, STRINGIFY(ptr), __func__)) return -1;
    size_t ntotaltowrite = size * count;
    if (file_writer->storage_type == BLOB) {
        BlobFileWriter *blob_file_writer = file_writer->blob_file_writer;
        Azure::Storage::Blobs::BlockBlobClient bbclient = blob_file_writer->blob_client.AsBlockBlobClient();
        ostringstream oss;
        oss << setfill('0') << setw(64) << blob_file_writer->block_ids.size();
        string block_id_in_base10 = oss.str();
        vector<uint8_t> block_id_in_base_10_as_vec(block_id_in_base10.begin(), block_id_in_base10.end());
        string block_id_in_base64 = Azure::Core::Convert::Base64Encode(block_id_in_base_10_as_vec);
        Azure::Core::IO::MemoryBodyStream body_stream(static_cast<const uint8_t *>(ptr), ntotaltowrite);
        bbclient.StageBlock(block_id_in_base64, body_stream);
        blob_file_writer->block_ids.push_back(block_id_in_base64);
    } else if (file_writer->storage_type == FILE_SHARE) {
        FileShareFileWriter *file_share_file_writer = file_writer->file_share_file_writer;
        Azure::Storage::Files::Shares::Models::FileHttpHeaders http_headers;
        Azure::Storage::Files::Shares::Models::FileSmbProperties smb_properties;
        Azure::Storage::Files::Shares::SetFilePropertiesOptions opts;
        Azure::Core::IO::MemoryBodyStream body_stream(static_cast<const uint8_t *>(ptr), ntotaltowrite);
        opts.Size = file_share_file_writer->current_position + ntotaltowrite;
        file_share_file_writer->share_file_client.SetProperties(http_headers, smb_properties, opts);
        file_share_file_writer->share_file_client.UploadRange(static_cast<int64_t>(file_share_file_writer->current_position), body_stream);
        file_share_file_writer->current_position += ntotaltowrite;
    }
    *result = ntotaltowrite;
    return 0;
}

int FFlush(const FileWriter *file_writer) {
    if (CheckNotNull(file_writer, STRINGIFY(file_writer), __func__)) return -1;
    if (file_writer->storage_type == BLOB) {
        file_writer->blob_file_writer->blob_client.AsBlockBlobClient().CommitBlockList(file_writer->blob_file_writer->block_ids);
    } else if (file_writer->storage_type == FILE_SHARE) {
        file_writer->file_share_file_writer->share_file_client.ForceCloseAllHandles();
    }
    return 0;
}

int FClose(FileWriter *file_writer) {
    if (CheckNotNull(file_writer, STRINGIFY(file_writer), __func__)) return -1;
    if (FFlush(file_writer)) return -1;
    if (UnregisterFileStream(&GetState()->open_file_streams, static_cast<void *>(file_writer))) return -1;
    if (file_writer->storage_type == BLOB) delete file_writer->blob_file_writer;
    else /* FILE SHARE */ delete file_writer->file_share_file_writer;
    delete file_writer;
    return 0;
}

int PopulateBlobFileWriterInWriteMode(BlobFileWriter *file_writer, const string &url) {
    if (CheckNotNull(file_writer, STRINGIFY(file_writer), __func__)) return -1;
    if (GetBlobClient(&file_writer->blob_client, url)) return -1;
    file_writer->url = url;
    file_writer->current_position = 0ULL;
    file_writer->block_ids.clear();
    return 0;
}

int PopulateBlobFileWriterInAppendMode(BlobFileWriter *file_writer, const string &url, size_t file_size) {
    if (CheckNotNull(file_writer, STRINGIFY(file_writer), __func__)) return -1;
    if (GetBlobClient(&file_writer->blob_client, url)) return -1;
    file_writer->url = url;
    file_writer->block_ids.clear();
    file_writer->current_position = file_size;
    try {
        auto block_list_request_response = file_writer->blob_client.AsBlockBlobClient().GetBlockList();
        vector<Azure::Storage::Blobs::Models::BlobBlock> blocks = block_list_request_response.Value.CommittedBlocks;
        transform(blocks.begin(), blocks.end(), back_inserter(file_writer->block_ids), [](const auto &block) { return block.Name; });
    } catch (const Azure::Storage::StorageException &) {}
    return 0;
}

int PopulateFileShareFileWriterInWriteMode(FileShareFileWriter *file_writer, const string &url) {
    if (CheckNotNull(file_writer, STRINGIFY(file_writer), __func__)) return -1;
    if (GetFileClient(&file_writer->share_file_client, url)) return -1;
    file_writer->url = url;
    file_writer->current_position = 0ULL;
    return 0;
}

int PopulateFileShareFileWriterInAppendMode(FileShareFileWriter *file_writer, const string &url, size_t file_size) {
    if (CheckNotNull(file_writer, STRINGIFY(file_writer), __func__)) return -1;
    if (GetFileClient(&file_writer->share_file_client, url)) return -1;
    file_writer->url = url;
    file_writer->current_position = file_size;
    return 0;
}

int FOpenForReading(void **result, const vector<string> &fragment_urls, StorageType storage_type) {
    unique_ptr<FileReader> file_reader = make_unique<FileReader>();
    void *handle = static_cast<void *>(file_reader.get());
    if (PopulateFileReader(file_reader.get(), storage_type, fragment_urls)) return -1;
    if (RegisterFileStream(&GetState()->open_file_streams, handle, FileStreamMode::READ)) return -1;
    file_reader.release();
    *result = handle;
    return 0;
}

int FOpenForWriting(void **result, const string &url, StorageType storage_type) {
    unique_ptr<FileWriter> file_writer = make_unique<FileWriter>();
    void *handle = static_cast<void *>(file_writer.get());
    file_writer->storage_type = storage_type;
    unique_ptr<BlobFileWriter> blob_file_writer;
    unique_ptr<FileShareFileWriter> file_share_file_writer;
    if (storage_type == BLOB) {
        blob_file_writer = make_unique<BlobFileWriter>();
        file_writer->blob_file_writer = blob_file_writer.get();
        if (PopulateBlobFileWriterInWriteMode(file_writer->blob_file_writer, url)) return -1;
    } else /* FILE SHARE */ {
        file_share_file_writer = make_unique<FileShareFileWriter>();
        file_writer->file_share_file_writer = file_share_file_writer.get();
        if (PopulateFileShareFileWriterInWriteMode(file_writer->file_share_file_writer, url)) return -1;
        file_writer->file_share_file_writer->share_file_client.Create(0LL);
    }
    if (RegisterFileStream(&GetState()->open_file_streams, handle, FileStreamMode::WRITE)) return -1;
    blob_file_writer.release();
    file_share_file_writer.release();
    file_writer.release();
    *result = handle;
    return 0;
}

int FOpenForAppending(void **result, const string &url, StorageType storage_type, const Azure::Core::Url &azure_url) {
    vector<string> fragment_urls;
    if (ResolveFragmentUrls(&fragment_urls, storage_type, azure_url)) return -1;
    bool already_exists;
    size_t file_size;
    if (fragment_urls.empty()) {
        already_exists = false;
        file_size = 0ULL;
    } else {
        already_exists = true;
        FileReader tmp_file_reader;
        if (PopulateFileReader(&tmp_file_reader, storage_type, fragment_urls)) return -1;
        file_size = tmp_file_reader.total_size;
    }
    unique_ptr<FileWriter> file_writer = make_unique<FileWriter>();
    void *handle = static_cast<void *>(file_writer.get());
    file_writer->storage_type = storage_type;
    unique_ptr<BlobFileWriter> blob_file_writer;
    unique_ptr<FileShareFileWriter> file_share_file_writer;
    if (storage_type == BLOB) {
        blob_file_writer = make_unique<BlobFileWriter>();
        file_writer->blob_file_writer = blob_file_writer.get();
        if (PopulateBlobFileWriterInAppendMode(file_writer->blob_file_writer, url, file_size)) return -1;
    } else /* FILE SHARE */ {
        file_share_file_writer = make_unique<FileShareFileWriter>();
        file_writer->file_share_file_writer = file_share_file_writer.get();
        if (PopulateFileShareFileWriterInAppendMode(file_writer->file_share_file_writer, url, file_size)) return -1;        
        if (!already_exists) file_writer->file_share_file_writer->share_file_client.Create(0LL);
    }
    if (RegisterFileStream(&GetState()->open_file_streams, handle, FileStreamMode::APPEND)) return -1;
    blob_file_writer.release();
    file_share_file_writer.release();
    file_writer.release();
    *result = handle;
    return 0;
}

}