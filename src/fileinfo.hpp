#pragma once

namespace az
{
	class FileInfo;
	class FileInfo::NoFilePartInfoError;
	struct FileInfo::PartInfo;
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
		class NoFilePartInfoError;
		struct PartInfo;

		FileInfo();
		FileInfo(const std::vector<FilePartInfo>& filePartInfo);
		const std::string& GetHeader() const;
		const std::vector<PartInfo>& GetParts() const;
		size_t GetSize() const;
		size_t GetFilePartIndexOfUserOffset(size_t nUserOffset) const;

		class NoFilePartInfoError : public Error
		{
		public:
			virtual const char* what() const noexcept override
			{
				return "no part info found in file info";
			}
		};

		struct PartInfo
		{
			size_t nRealOffset;
			size_t nUserOffset;
			size_t nContentSize;
		};

	private:
		std::string sHeader;
		std::vector<PartInfo> parts;
		size_t nSize;
	};

	struct FilePartInfo
	{
		std::string sHeader;
		size_t nSize;
	};
}
