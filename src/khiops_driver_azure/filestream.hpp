#pragma once

#include <vector>
#include <string>
#include <azure/core.hpp>
#include "khiops_driver_azure/util.hpp"

namespace khiops_driver_azure {

struct FileReader {
    struct Fragment {
        // The URL of the remote object representing a fragment of the whole file.
        std::string url;
        // The user offset is the start position of this fragment in the whole
        // file as seen by the user. If the file contains a header that is
        // repeated in each fragment, the user only sees the header at the
        // beginning of the first fragment. The user offset is always zero for
        // the first fragment.
        size_t user_offset;
        // The content size includes the header length only for the first fragment.
        size_t content_size;
        // The version of the file is used to detect if the file has been modified before a reading.
        Azure::ETag version;
    };
    // Fragments can have an empty content or may be considered to have an empty content if there is a repeated header, if they are not the first fragment and there content consists only of the header.
    // Empty fragments are not removed from the vector below because they can be modified between readings and become non-empty.
    // This is a case where the reading will abort in error due to a file fragment version mismatch.
    // Keeping the empty fragments allow us to detect that error case.
    std::vector<Fragment> fragments;
    // Total size of the file as seen by the user, that is, if there is a repeated header, its length is included only in the size of the first fragment.
    size_t total_size;
    // The header length is zero if there is no repeated header in this file.
    size_t header_length;
    // The current position in the file, as seen by the user.
    size_t current_position;
    StorageType storage_type;
};

int PopulateFileReader(FileReader *file_reader, StorageType storage_type, const std::vector<std::string> &fragment_urls);
// Get the index of the fragment to which the given user offset belongs.
int FragmentIndexOfUserOffset(size_t *result, const FileReader &file_reader, size_t user_offset);
// Read a given number of bytes from a file reader to a buffer.
int FRead(size_t *result, void *ptr, FileReader *file_reader, size_t size, size_t count);
int FSeek(FileReader *file_reader, long long int offset, int whence);
int FClose(FileReader *file_reader);

struct BlobFileWriter {
    std::string url;
    size_t current_position;
    std::vector<std::string> block_ids;
    Azure::Storage::Blobs::BlobClient blob_client;
    inline BlobFileWriter(): blob_client("") {}
};

int PopulateBlobFileWriterInWriteMode(BlobFileWriter *file_writer, const std::string &url);
int PopulateBlobFileWriterInAppendMode(BlobFileWriter *file_writer, const std::string &url, size_t file_size);

struct FileShareFileWriter {
    std::string url;
    size_t current_position;
    Azure::Storage::Files::Shares::ShareFileClient share_file_client;
    inline FileShareFileWriter(): share_file_client("") {}
};

int PopulateFileShareFileWriterInWriteMode(FileShareFileWriter *file_writer, const std::string &url);
int PopulateFileShareFileWriterInAppendMode(FileShareFileWriter *file_writer, const std::string &url, size_t file_size);

int FOpenForReading(void **result, const std::vector<std::string> &fragment_urls, StorageType storage_type);
int FOpenForWriting(void **result, const std::string &url, StorageType storage_type);
int FOpenForAppending(void **result, const std::string &url, StorageType storage_type, const Azure::Core::Url &azure_url);

struct FileWriter {
    StorageType storage_type;
    BlobFileWriter *blob_file_writer;
    FileShareFileWriter *file_share_file_writer;
};

int FWrite(size_t *result, FileWriter *file_writer, const void *ptr, size_t size, size_t count);
int FFlush(const FileWriter *file_writer);
int FClose(FileWriter *file_writer);

}