#pragma once

namespace az
{
	class FileInfo;
	class FileInfo::NoOffsetError;
	struct FilePartInfo;
}

#include <string>
#include <cstddef>
#include <vector>
#include "exception.hpp"

namespace az
{
	class FileInfo
	{
	public:
		FileInfo(const std::vector<FilePartInfo>& filePartInfo);
		const std::string& GetHeader() const;
		const std::vector<size_t>& GetOffsets() const;
		size_t GetFilePartIndexOfPos(size_t nPos) const;

		class NoOffsetError : public Error
		{
		public:
			virtual const char* what() const noexcept override
			{
				return "no offset found in file info";
			}
		};

	private:
		std::string sHeader;
		std::vector<size_t> offsets;
	};

	struct FilePartInfo
	{
		std::string sHeader;
		size_t nSize;
	};
}
