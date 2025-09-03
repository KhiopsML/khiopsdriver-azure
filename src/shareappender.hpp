#pragma once

namespace az
{
	class ShareWriter;
}

#include <cstddef>
#include <memory>
#include <azure/storage/files/shares/share_file_client.hpp>
#include <azure/core/io/body_stream.hpp>
#include "fileappender.hpp"

namespace az
{
	class ShareWriter : public FileAppender
	{
	public:
		ShareWriter(Azure::Storage::Files::Shares::ShareFileClient&& client);

		void Close() override;
		size_t Write(const void* source, size_t nSize, size_t nCount) override;

	private:
		Azure::Storage::Files::Shares::ShareFileClient client;
		std::unique_ptr<Azure::Core::IO::BodyStream> bodyStream;
	};
}
