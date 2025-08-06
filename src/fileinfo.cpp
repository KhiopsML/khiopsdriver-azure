#include "fileinfo.hpp"
#include <algorithm>

using namespace std;

namespace az
{
	static string GetHeaderOfFile(const vector<FilePartInfo>& filePartInfo);
	static vector<FileInfo::PartInfo> GetFileParts(const vector<FilePartInfo>& filePartInfo, const string& sHeader);

	FileInfo::FileInfo(const vector<FilePartInfo>& filePartInfo):
		sHeader(GetHeaderOfFile(filePartInfo)),
		parts(GetFileParts(filePartInfo, sHeader))
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

	static vector<FileInfo::PartInfo> GetFileParts(const vector<FilePartInfo>& filePartInfo, const string& sHeader)
	{
		vector<FileInfo::PartInfo> parts;
		size_t nUserOffset = 0;
		for (const auto& partInfo : filePartInfo)
		{
			parts.push_back(FileInfo::PartInfo { nUserOffset + sHeader.length(), partInfo.nSize - sHeader.length(), nUserOffset });
			nUserOffset += partInfo.nSize;
		}
		return parts;
	}
}
