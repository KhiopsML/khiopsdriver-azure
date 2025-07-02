#pragma once

namespace az
{
	class EmulatedBlobAccessor;
}

#include "fileaccessor.hpp"
#include "blobaccessor.hpp"
#include "emulatedfileaccessor.hpp"
#include <azure/core.hpp>
#include <azure/storage/blobs.hpp>

namespace az
{
	class EmulatedBlobAccessor : public BlobAccessor, public EmulatedFileAccessor
	{
	public:
		EmulatedBlobAccessor(const Azure::Core::Url& url, const string& sConnectionString);
		~EmulatedBlobAccessor();

	protected:
		Azure::Storage::Blobs::BlobClient GetBlobClient() const;
	};
}
