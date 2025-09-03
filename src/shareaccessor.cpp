#include "shareaccessor.hpp"
#include <queue>
#include <deque>
#include <numeric>
#include <algorithm>
#include "exception.hpp"
#include "sharepathresolve.hpp"
#include "fileinfo.hpp"

using namespace std;
using ShareFileClient = Azure::Storage::Files::Shares::ShareFileClient;
using ShareDirectoryClient = Azure::Storage::Files::Shares::ShareDirectoryClient;
using Url = Azure::Core::Url;
using TransportException = Azure::Core::Http::TransportException;
using ListFilesAndDirectoriesOptions = Azure::Storage::Files::Shares::ListFilesAndDirectoriesOptions;

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
			vector<ShareFileClient> files = ListFiles();
			if (files.empty())
			{
				throw NoFileError(GetUrl().GetAbsoluteUrl());
			}
			return ShareFileInfo(files).GetSize();
		}
	}

	const unique_ptr<FileReader>& ShareAccessor::OpenForReading() const
	{
		// TODO: Implement
		auto r = unique_ptr<FileReader>();
		return r;
	}

	const unique_ptr<FileOutputStream>& ShareAccessor::OpenForWriting() const
	{
		// TODO: Implement
		auto r = unique_ptr<FileOutputStream>();
		return r;
	}

	const unique_ptr<FileOutputStream>& ShareAccessor::OpenForAppending() const
	{
		// TODO: Implement
		auto r = unique_ptr<FileOutputStream>();
		return r;
	}

	void ShareAccessor::Remove() const
	{
		vector<ShareFileClient> files = ListFiles();
		if (files.empty())
		{
			throw NoFileError(GetUrl().GetAbsoluteUrl());
		}
		for (const ShareFileClient& file : files)
		{
			const string sFileUrl = file.GetUrl();
			if (!file.Delete().Value.Deleted)
			{
				throw DeletionError(sFileUrl);
			}
		}
	}

	void ShareAccessor::MkDir() const
	{
		ShareDirectoryClient dirClient = GetDirClient();
		vector<string> path = GetPath();
		string sPathFragment;

		for (size_t i = 0; i < path.size(); i++)
		{
			sPathFragment = path[i];
			dirClient = dirClient.GetSubdirectoryClient(sPathFragment);
			ListFilesAndDirectoriesOptions opts;
			opts.Prefix = sPathFragment;

			bool bAlreadyExisting = false;
			for (auto pagedResponse = dirClient.ListFilesAndDirectories(opts); pagedResponse.HasPage(); pagedResponse.MoveToNextPage())
			{
				if (find_if(pagedResponse.Directories.begin(), pagedResponse.Directories.end(), [sPathFragment](const auto& dirItem)
					{
						return dirItem.Name == sPathFragment;
					}) != pagedResponse.Directories.end()
				)
				{
					bAlreadyExisting = true;
					break;
				}
			}

			if (i < path.size() - 1 && !bAlreadyExisting)
			{
				throw IntermediateDirNotFoundError(dirClient.GetUrl());
			}

			if (i == path.size() - 1 && bAlreadyExisting)
			{
				throw DirAlreadyExistsError(dirClient.GetUrl());
			}
		}

		if (!dirClient.Create().Value.Created)
		{
			throw CreationError(GetUrl().GetAbsoluteUrl());
		}
	}

	void ShareAccessor::RmDir() const
	{
		vector<ShareDirectoryClient> dirs = ListDirs();
		if (dirs.empty())
		{
			throw NoFileError(GetUrl().GetAbsoluteUrl());
		}
		for (const ShareDirectoryClient& dir : dirs)
		{
			const string sDirUrl = dir.GetUrl();
			if (!dir.Delete().Value.Deleted)
			{
				throw DeletionError(sDirUrl);
			}
		}
	}

	size_t ShareAccessor::GetFreeDiskSpace() const
	{
		return 5LL * 1024LL * 1024LL * 1024LL * 1024LL;
	}

	void ShareAccessor::CopyTo(const string& destUrl) const
	{
		// TODO: Implement
	}

	void ShareAccessor::CopyFrom(const string& sourceUrl) const
	{
		// TODO: Implement
	}

	ShareAccessor::ShareAccessor(const Url& url, const function<const unique_ptr<FileReader>& (unique_ptr<FileReader>)>& registerReader, const function<const unique_ptr<FileWriter>& (unique_ptr<FileWriter>)>& registerWriter, const function<const unique_ptr<FileAppender>& (unique_ptr<FileAppender>)>& registerAppender) :
		FileAccessor(url, registerReader, registerWriter, registerAppender)
	{
	}

	vector<ShareDirectoryClient> ShareAccessor::ListDirs() const
	{
		vector<string> path = GetPath();

		try
		{
			return ResolveDirsPathRecursively(GetDirClient(), queue<string, deque<string>>(deque<string>(path.begin(), path.end())));
		}
		catch (const TransportException& exc)
		{
			throw NetworkError();
		}
	}

	vector<ShareFileClient> ShareAccessor::ListFiles() const
	{
		vector<string> path = GetPath();

		try
		{
			return ResolveFilesPathRecursively(GetDirClient(), queue<string, deque<string>>(deque<string>(path.begin(), path.end())));
		}
		catch (const TransportException& exc)
		{
			throw NetworkError();
		}
	}
}
