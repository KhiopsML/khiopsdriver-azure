#pragma once

namespace az
{
	class CloudShareAccessor;
}

#include "fileaccessor.hpp"
#include "shareaccessor.hpp"
#include "cloudfileaccessor.hpp"
#include <azure/core.hpp>
#include <azure/storage/files/shares.hpp>

namespace az
{
	class CloudShareAccessor : public ShareAccessor, public CloudFileAccessor
	{
	public:
		CloudShareAccessor(const Azure::Core::Url& url);
		~CloudShareAccessor();

	protected:
		Azure::Storage::Files::Shares::ShareFileClient GetShareFileClient() const override;
	};
}
