#include "shareaccessor.hpp"
#include <queue>
#include <deque>
#include <string>
#include "util/urlresolve.hpp"

namespace az
{
	ShareAccessor::~ShareAccessor()
	{
	}

	bool ShareAccessor::Exists() const
	{
		if (HasDirUrl())
		{
			CheckDirUrl();
		}
		else
		{
			CheckFileUrl();
		}
		return !ResolveUrl().empty();
	}

	size_t ShareAccessor::GetSize() const
	{
		// TODO: Implement
		return 0;
	}

	FileStream ShareAccessor::Open(char mode) const
	{
		// TODO: Implement
		return FileStream();
	}

	void ShareAccessor::Remove() const
	{
		// TODO: Implement
	}

	void ShareAccessor::MkDir() const
	{
		// TODO: Implement
	}

	void ShareAccessor::RmDir() const
	{
		// TODO: Implement
	}

	size_t ShareAccessor::GetFreeDiskSpace() const
	{
		// TODO: Implement
		return 0;
	}

	void ShareAccessor::CopyTo(const Azure::Core::Url& destUrl) const
	{
		// TODO: Implement
	}

	void ShareAccessor::CopyFrom(const Azure::Core::Url& sourceUrl) const
	{
		// TODO: Implement
	}

	ShareAccessor::ShareAccessor(const Azure::Core::Url& url):
		FileAccessor(url)
	{
	}

	vector<string> ShareAccessor::ResolveUrl() const
	{
		vector<string> path = GetPath();
		return ResolveUrlRecursively(
			GetDirClient(),
			queue<string, deque<string>>(deque<string>(path.begin(), path.end())),
			HasDirUrl(),
			""
		);
	}
}
