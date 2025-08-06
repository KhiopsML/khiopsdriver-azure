#include "fileinfo.hpp"
#include <algorithm>

using namespace std;

namespace az
{
	static string GetHeaderOfFile(const vector<FilePartInfo>& filePartInfo);
	static vector<FileInfo::PartInfo> GetFileParts(const vector<FilePartInfo>& filePartInfo, size_t nHeaderLen);

	FileInfo::FileInfo(const vector<FilePartInfo>& filePartInfo):
		sHeader(GetHeaderOfFile(filePartInfo)),
		parts(GetFileParts(filePartInfo, sHeader.length()))
	{
	}

	const string& FileInfo::GetHeader() const
	{
		return sHeader;
	}

	const vector<FileInfo::PartInfo>& FileInfo::GetParts() const
	{
		return parts;
	}

	size_t FileInfo::GetFilePartIndexOfUserOffset(size_t nUserOffset) const
	{
		if (parts.empty())
		{
			throw NoFilePartInfoError();
		}
		return find_if(parts.begin(), parts.end(), [nUserOffset](const auto& part) { return nUserOffset < part.nUserOffset; }) - 1 - parts.begin();
	}

	static string GetHeaderOfFile(const vector<FilePartInfo>& filePartInfo)
	{
		string sFirstHeader = filePartInfo.front().sHeader;
		return sFirstHeader.empty()
			|| any_of(filePartInfo.begin() + 1, filePartInfo.end(), [sFirstHeader](const auto& partInfo) { return partInfo.sHeader != sFirstHeader; })
			? string()
			: sFirstHeader;
	}

	static vector<FileInfo::PartInfo> GetFileParts(const vector<FilePartInfo>& filePartInfo, size_t nHeaderLen)
	{
		vector<FileInfo::PartInfo> parts;
		size_t nRealOffset = 0;
		size_t nUserOffset = 0;
		size_t nContentSize;
		for (const auto& partInfo : filePartInfo)
		{
			nContentSize = partInfo.nSize - nHeaderLen;
			parts.push_back(FileInfo::PartInfo { nRealOffset, nUserOffset, nContentSize });
			nRealOffset += partInfo.nSize;
			nUserOffset += nContentSize;
		}
		return parts;
	}
}
