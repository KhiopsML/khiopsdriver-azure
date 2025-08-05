#include "fileinfo.hpp"
#include <algorithm>

using namespace std;

namespace az
{
	FileInfo::FileInfo(const vector<FilePartInfo>& filePartInfo):
		offsets()
	{
		bool bFirstIter = true;
		bool bMayHaveHeader = true;
		size_t nOffset = 0;
		for (const auto& partInfo: filePartInfo)
		{
			if (bFirstIter)
			{
				if (partInfo.sHeader.empty())
				{
					bMayHaveHeader = false;
				}
				else
				{
					sHeader = partInfo.sHeader;
				}
				bFirstIter = false;
			}
			else if (bMayHaveHeader && partInfo.sHeader != sHeader)
			{
				sHeader = string();
				bMayHaveHeader = false;
			}
			offsets.push_back(nOffset);
			nOffset += partInfo.nSize;
		}
	}

	const string& FileInfo::GetHeader() const
	{
		return sHeader;
	}

	const vector<size_t>& FileInfo::GetOffsets() const
	{
		return offsets;
	}

	size_t FileInfo::GetFilePartIndexOfPos(size_t nPos) const
	{
		if (offsets.empty())
		{
			throw NoOffsetError();
		}
		return find_if(offsets.begin(), offsets.end(), [nPos](size_t offset) { return nPos < offset; }) - 1 - offsets.begin();
	}
}
