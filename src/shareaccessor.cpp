#include "shareaccessor.hpp"
#include <queue>
#include <deque>
#include <numeric>
#include "exception.hpp"
#include "sharepathresolve.hpp"

using namespace std;

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
		if (HasDirUrl())
		{
			throw GettingSizeOfDirError();
		}
		else
		{
			vector<Azure::Storage::Files::Shares::ShareFileClient> files = ListFiles();
			if (files.empty())
			{
				throw NoFileError(GetUrl().GetAbsoluteUrl());
			}
			return accumulate(files.begin(), files.end(), 0,
				[](size_t total, const Azure::Storage::Files::Shares::ShareFileClient& file)
				{
					return total + file.GetProperties().Value.FileSize;
				}
			);
		}
	}

	std::unique_ptr<FileReader> ShareAccessor::OpenForReading() const
	{
		// TODO: Implement
		return nullptr;
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
