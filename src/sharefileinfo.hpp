#pragma once

namespace az
{
	class ShareFileInfo;
}

#include <vector>
#include <memory>
#include <azure/storage/files/shares/share_file_client.hpp>
#include <azure/core/io/body_stream.hpp>
#include "fileinfo.hpp"

namespace az
{
	class ShareFileInfo : public FileInfo
	{
	public:
		ShareFileInfo(const std::vector<Azure::Storage::Files::Shares::ShareFileClient>& clients);
		std::vector<std::unique_ptr<Azure::Core::IO::BodyStream>>& GetBodyStreams();

	private:
		std::vector<std::unique_ptr<Azure::Core::IO::BodyStream>> bodyStreams;
	};
}
