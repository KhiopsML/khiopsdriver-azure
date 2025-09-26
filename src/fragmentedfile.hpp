// Remote file abstraction that understands a file might have been split into multiple remote files.
// This is also used for monolythic files, which are considered as single-fragment files.

#pragma once

#include <cstddef>
#include <vector>
#include <azure/storage/blobs/blob_client.hpp>
#include <azure/storage/files/shares/share_file_client.hpp>
#include <azure/core/io/body_stream.hpp>
#include "exception.hpp"
#include "storagetype.hpp"
#include "objectclient.hpp"

namespace az
{
	class FragmentedFile
	{
	public:
		struct Fragment
		{
			size_t nUserOffset;
			size_t nContentSize;
			ObjectClient client;
			Azure::ETag etag;

			Fragment(size_t nContentSize, const ObjectClient& client, const Azure::ETag& etag);
			Fragment(Fragment&& source);
			Fragment& operator=(Fragment&& source);
		};

		class NoFragmentError : public Error
		{
		public: inline NoFragmentError() : Error("no fragment found in fragmented file") {}
		};

		FragmentedFile();
		FragmentedFile(const std::vector<Azure::Storage::Blobs::BlobClient>& clients);
		FragmentedFile(const std::vector<Azure::Storage::Files::Shares::ShareFileClient>& clients);
		FragmentedFile(const std::vector<ObjectClient>& clients);
		FragmentedFile(FragmentedFile&& source);
		~FragmentedFile();
		size_t GetSize() const;
		size_t GetHeaderLen() const;
		const Fragment& GetFragment(size_t nIndex) const;
		size_t GetFragmentIndexOfUserOffset(size_t nUserOffset) const;
		
	private:
		StorageType storageType;
		size_t nHeaderLen;
		size_t nSize;
		std::vector<Fragment> fragments;
	};
}
