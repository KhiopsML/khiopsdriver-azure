#include "shareaccessor.hpp"
#include <queue>
#include <deque>
#include "exception.hpp"
#include "sharepathresolve.hpp"

namespace az
{
	ShareAccessor::~ShareAccessor()
	{
	}

	bool ShareAccessor::Exists() const
	{
		if (HasDirUrl())
		{
			return !ListDirs().empty();
		}
		else
		{
			return !ListFiles().empty();
		}
	}

	size_t ShareAccessor::GetSize() const
	{
		return 0; // TODO: Implement.
	}
	//size_t ShareAccessor::GetSize() const
	//{
	//	if (HasDirUrl())
	//	{
	//		// TODO: What should it do?
	//	}
	//	else
	//	{
	//		vector<Azure::Storage::Files::Shares::Models::FileItem> files = ResolveUrl();
	//		return accumulate(files.begin(), files.end(), 0,
	//			[](size_t acc, const Azure::Storage::Files::Shares::Models::FileItem& elem)
	//			{
	//				return acc + elem.BlobSize;
	//			}
	//		);
	//	}
	//}

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

	//vector<string> ShareAccessor::ResolveUrl() const
	//{
	//	vector<string> path = GetPath();
	//	return ResolveUrlRecursively(
	//		GetDirClient(),
	//		queue<string, deque<string>>(deque<string>(path.begin(), path.end())),
	//		HasDirUrl(),
	//		""
	//	);
	//}

	vector<Azure::Storage::Files::Shares::ShareDirectoryClient> ShareAccessor::ListDirs() const
	{
		vector<string> path = GetPath();

		try
		{
			return ResolveDirsPathRecursively(GetDirClient(), queue<string, deque<string>>(deque<string>(path.begin(), path.end())));
		}
		catch (const Azure::Core::Http::TransportException& exc)
		{
			throw NetworkError();
		}
	}

	vector<Azure::Storage::Files::Shares::ShareFileClient> ShareAccessor::ListFiles() const
	{
		vector<string> path = GetPath();

		try
		{
			return ResolveFilesPathRecursively(GetDirClient(), queue<string, deque<string>>(deque<string>(path.begin(), path.end())));
		}
		catch (const Azure::Core::Http::TransportException& exc)
		{
			throw NetworkError();
		}
	}
}
