#pragma once

#include <memory>
#include <string>
#include <vector>

//#include <google/cloud/storage/object_write_stream.h>

#if defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__))
#define __unix_or_mac__
#else
#define __windows__
#endif

#ifdef __unix_or_mac__
#define VISIBLE __attribute__((visibility("default")))
#else
/* Windows Visual C++ only */
#define VISIBLE __declspec(dllexport)
#endif

/* Use of C linkage from C++ */
#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

	//VISIBLE void test_setClient(::google::cloud::storage::Client && mock_client);

	VISIBLE void test_unsetClient();

	VISIBLE void* test_getActiveHandles();

	VISIBLE void* test_addReaderHandle(const std::string& bucket, const std::string& object, long long offset,
					   long long commonHeaderLength, const std::vector<std::string>& filenames,
					   const std::vector<long long int>& cumulativeSize, long long total_size);

	VISIBLE void* test_addWriterHandle(bool appendMode = false, bool create_with_mock_client = false,
					   std::string bucketname = {}, std::string objectname = {});

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#include "azure/storage/blobs/block_blob_client.hpp"

namespace azureplugin
{
constexpr int kSuccess{1};
constexpr int kFailure{0};

constexpr int kCloseSuccess{0};
constexpr int kCloseEOF{-1};

constexpr int kFalse{0};
constexpr int kTrue{1};

constexpr int kBadSize{-1};

using tOffset = long long;

struct MultiPartFile
{
	std::string bucketname_;
	std::string filename_;
	tOffset offset_{0};
	// Added for multifile support
	tOffset commonHeaderLength_{0};
	std::vector<std::string> filenames_;
	std::vector<tOffset> cumulativeSize_;
	tOffset total_size_{0};

	MultiPartFile() = default;
	MultiPartFile(std::string bucket, std::string filename, tOffset off, tOffset common_header_length,
		      std::vector<std::string>&& filenames, std::vector<tOffset>&& cumulativeSize)
	    : bucketname_{std::move(bucket)}, filename_{std::move(filename)}, offset_{off},
	      commonHeaderLength_{common_header_length}, filenames_{std::move(filenames)},
	      cumulativeSize_{std::move(cumulativeSize)}, total_size_{cumulativeSize_.back()}
	{
	}
};

struct WriteFile
{
	Azure::Storage::Blobs::BlockBlobClient client_;
	std::vector<std::string> block_ids_list_;
	std::string bucketname_;
	std::string filename_;

	explicit WriteFile(std::string bucket, std::string filename, Azure::Storage::Blobs::BlockBlobClient&& client)
	: client_{std::move(client)}
	, bucketname_{std::move(bucket)}
	, filename_{std::move(filename)}
	{}
};

using Reader = MultiPartFile;
using Writer = WriteFile;
using ReaderPtr = std::unique_ptr<Reader>;
using WriterPtr = std::unique_ptr<Writer>;

template <typename Stream> using StreamPtr = std::unique_ptr<Stream>;

template <typename Stream> using StreamVec = std::vector<StreamPtr<Stream>>;

template <typename Stream> using StreamIt = typename StreamVec<Stream>::iterator;

bool operator==(const MultiPartFile& op1, const MultiPartFile& op2)
{
	return (op1.bucketname_ == op2.bucketname_ && op1.filename_ == op2.filename_ && op1.offset_ == op2.offset_ &&
		op1.commonHeaderLength_ == op2.commonHeaderLength_ && op1.filenames_ == op2.filenames_ &&
		op1.cumulativeSize_ == op2.cumulativeSize_ && op1.total_size_ == op2.total_size_);
}

bool operator==(const WriteFile& op1, const WriteFile& op2)
{
	return (op1.bucketname_ == op2.bucketname_ && op1.filename_ == op2.filename_);
}
} // namespace azureplugin