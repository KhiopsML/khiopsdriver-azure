#include "shareaccessor.hpp"
#include <queue>
#include <deque>
#include <numeric>
#include <algorithm>
#include <fstream>
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
			throw InvalidOperationForDirError(Operation::GET_SIZE);
		}
		else
		{
			vector<ShareFileClient> files = ListFiles();
			if (files.empty())
			{
				throw NoFileError(GetUrl().GetAbsoluteUrl());
			}
			return FileInfo(move(files)).GetSize();
		}
	}

	FileReader& ShareAccessor::OpenForReading() const
	{
		if (HasDirUrl())
		{
			throw InvalidOperationForDirError(Operation::READ);
		}
		else
		{
			vector<ShareFileClient> files = ListFiles();
			return RegisterReader(move(FileReader(move(files))));
		}
	}

	FileOutputStream& ShareAccessor::OpenForWriting() const
	{
		if (HasDirUrl())
		{
			throw InvalidOperationForDirError(Operation::WRITE);
		}
		else
		{
			CheckParentDirExists();
			return RegisterWriter(move(FileOutputStream(FileOutputMode::WRITE, move(GetFileClient()))));
		}
	}

	FileOutputStream& ShareAccessor::OpenForAppending() const
	{
		if (HasDirUrl())
		{
			throw InvalidOperationForDirError(Operation::APPEND);
		}
		else
		{
			string sFilename = GetPath().back();
			ListFilesAndDirectoriesOptions opts;
			opts.Prefix = sFilename;
			bool bAlreadyExisting = false;
			for (auto pagedResponse = GetParentDir().ListFilesAndDirectories(opts); pagedResponse.HasPage(); pagedResponse.MoveToNextPage())
			{
				if (find_if(pagedResponse.Files.begin(), pagedResponse.Files.end(), [sFilename](const auto& fileItem)
					{
						return fileItem.Name == sFilename;
					}) != pagedResponse.Files.end()
				)
				{
					bAlreadyExisting = true;
					break;
				}
			}
			ShareFileClient client = GetFileClient();
			if (!bAlreadyExisting)
			{
				client.Create(0);
			}
			return RegisterWriter(move(FileOutputStream(FileOutputMode::APPEND, move(client))));
		}
	}

	void ShareAccessor::Remove() const
	{
		if (HasDirUrl())
		{
			throw InvalidOperationForDirError(Operation::REMOVE, "use driver_rmdir instead");
		}
		else
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
	}

	void ShareAccessor::MkDir() const
	{
		if (!HasDirUrl())
		{
			throw InvalidDirUrlPathError(GetUrl().GetPath());
		}
		else
		{
			string sNewDir = GetPath().back();
			ShareDirectoryClient parentDirClient = GetParentDir();

			ListFilesAndDirectoriesOptions opts;
			opts.Prefix = sNewDir;
			for (auto pagedResponse = parentDirClient.ListFilesAndDirectories(opts); pagedResponse.HasPage(); pagedResponse.MoveToNextPage())
			{
				if (find_if(pagedResponse.Directories.begin(), pagedResponse.Directories.end(), [sNewDir](const auto& dirItem)
					{
						return dirItem.Name == sNewDir;
					}) != pagedResponse.Directories.end()
				)
				{
					throw DirAlreadyExistsError(GetUrl().GetAbsoluteUrl());
				}
			}

			if (!parentDirClient.GetSubdirectoryClient(sNewDir).Create().Value.Created)
			{
				throw CreationError(GetUrl().GetAbsoluteUrl());
			}
		}
	}

	void ShareAccessor::RmDir() const
	{
		if (!HasDirUrl())
		{
			throw InvalidDirUrlPathError(GetUrl().GetPath());
		}
		else
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
	}

	size_t ShareAccessor::GetFreeDiskSpace() const
	{
		return 5LL * 1024LL * 1024LL * 1024LL * 1024LL;
	}

	void ShareAccessor::CopyTo(const string& destUrl) const
	{
		auto& reader = OpenForReading();
		constexpr size_t nBufferSize = 4ULL * 1024 * 1024; // TODO
		char* buffer = new char[nBufferSize];
		size_t nRead;
		ofstream ofs(destUrl, ios::binary);

		while ((nRead = reader.Read(buffer, 1, nBufferSize)) != 0ULL)
		{
			ofs.write(buffer, (streamsize)nRead);
		}

		delete[] buffer;
	}

	void ShareAccessor::CopyFrom(const string& sourceUrl) const
	{
		auto& writer = OpenForWriting();
		constexpr size_t nBufferSize = 4ULL * 1024 * 1024; // TODO
		char* buffer = new char[nBufferSize];
		size_t nRead;
		ifstream ifs(sourceUrl, ios::binary);

		for (;;)
		{
			ifs.read(buffer, nBufferSize);
			nRead = (size_t)ifs.gcount();
			if (nRead == 0)
			{
				break;
			}
			writer.Write(buffer, 1, nRead);
		}

		delete[] buffer;
	}

	ShareAccessor::ShareAccessor(const Azure::Core::Url& url, const function<FileReader& (FileReader&&)>& registerReader, const function<FileOutputStream& (FileOutputStream&&)>& registerWriter) :
		FileAccessor(url, registerReader, registerWriter)
	{
	}

	vector<ShareDirectoryClient> ShareAccessor::ListDirs() const
	{
		vector<string> path = GetPath();

		try
		{
			return ResolveDirsPathRecursively(GetDirClient(), queue<string, deque<string>>(deque<string>(path.begin(), path.end())));
		}
		catch (const TransportException&)
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
		catch (const TransportException&)
		{
			throw NetworkError();
		}
	}

	Azure::Storage::Files::Shares::ShareDirectoryClient ShareAccessor::GetParentDir() const
	{
		ShareDirectoryClient dirClient = GetDirClient();
		vector<string> path = GetPath();
		path.pop_back();

		for (string sPathFragment : path)
		{
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

			if (!bAlreadyExisting)
			{
				throw IntermediateDirNotFoundError(dirClient.GetUrl());
			}

			dirClient = dirClient.GetSubdirectoryClient(sPathFragment);
		}

		return dirClient;
	}

	void ShareAccessor::CheckParentDirExists() const
	{
		GetParentDir();
	}
}
