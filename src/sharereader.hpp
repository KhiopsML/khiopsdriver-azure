#pragma once

namespace az
{
	class ShareReader;
}

#include <cstddef>
#include <vector>
#include <azure/storage/files/shares/share_file_client.hpp>
#include "filereader.hpp"
#include "fileinfo.hpp"

namespace az
{
	class ShareReader : public FileReader
	{
	public:
		ShareReader(std::vector<Azure::Storage::Files::Shares::ShareFileClient>&& clients);
		~ShareReader();

		void Close() override;
		size_t Read(void* dest, size_t nSize, size_t nCount) override;
		void Seek(long long int nOffset, int nOrigin) override;

	private:
		std::vector<Azure::Storage::Files::Shares::ShareFileClient> clients;
		ShareFileInfo fileInfo;
	};
}
