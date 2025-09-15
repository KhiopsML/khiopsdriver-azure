#include "driver.hpp"
#include <regex>
#include <fstream>
#include <sstream>
#include <spdlog/spdlog.h>
#include <azure/core.hpp>
#include <azure/identity.hpp>
#include <azure/storage/blobs/blob_options.hpp>
#include <azure/storage/files/shares/share_options.hpp>
#include "util.hpp"
#include "exception.hpp"
#include "storagetype.hpp"
#include "blobpathresolve.hpp"
#include "sharepathresolve.hpp"

using namespace std;

namespace az
{
	ServiceRequest::ServiceRequest(const Azure::Core::Url azureUrl, StorageType storageType, bool bDir, const BlobInfo& blob, shared_ptr<Azure::Storage::StorageSharedKeyCredential> emulatedStorageCredential) :
		azureUrl(azureUrl),
		storageType(storageType),
		bEmulated(true),
		bDir(bDir)
	{
		new(&this->blob) BlobInfo{ blob.sAccountName, blob.sContainer, blob.sBlob };
		new(&this->emulatedStorageCredential) std::shared_ptr<Azure::Storage::StorageSharedKeyCredential>(emulatedStorageCredential);
	}

	ServiceRequest::ServiceRequest(const Azure::Core::Url azureUrl, StorageType storageType, bool bDir, const BlobInfo& blob, shared_ptr<Azure::Core::Credentials::TokenCredential> cloudStorageCredential) :
		azureUrl(azureUrl),
		storageType(storageType),
		bEmulated(false),
		bDir(bDir)
	{
		new(&this->blob) BlobInfo{ blob.sAccountName, blob.sContainer, blob.sBlob };
		new(&this->cloudStorageCredential) std::shared_ptr<Azure::Core::Credentials::TokenCredential>(cloudStorageCredential);
	}

	ServiceRequest::ServiceRequest(const Azure::Core::Url azureUrl, StorageType storageType, bool bDir, const ShareInfo& share, shared_ptr<Azure::Storage::StorageSharedKeyCredential> emulatedStorageCredential) :
		azureUrl(azureUrl),
		storageType(storageType),
		bEmulated(true),
		bDir(bDir)
	{
		new(&this->share) ShareInfo{ share.sShare, share.path };
		new(&this->emulatedStorageCredential) std::shared_ptr<Azure::Storage::StorageSharedKeyCredential>(emulatedStorageCredential);
	}

	ServiceRequest::ServiceRequest(const Azure::Core::Url azureUrl, StorageType storageType, bool bDir, const ShareInfo& share, shared_ptr<Azure::Core::Credentials::TokenCredential> cloudStorageCredential) :
		azureUrl(azureUrl),
		storageType(storageType),
		bEmulated(false),
		bDir(bDir)
	{
		new(&this->share) ShareInfo{ share.sShare, share.path };
		new(&this->cloudStorageCredential) std::shared_ptr<Azure::Core::Credentials::TokenCredential>(cloudStorageCredential);
	}

	ServiceRequest::ServiceRequest(const ServiceRequest& other) :
		azureUrl(other.azureUrl),
		storageType(other.storageType),
		bEmulated(other.bEmulated),
		bDir(other.bDir)
	{
		if (storageType == BLOB)
		{
			new(&this->blob) BlobInfo(other.blob);
		}
		else
		{
			new(&this->share) ShareInfo(other.share);
		}
		if (bEmulated)
		{
			new(&this->emulatedStorageCredential) std::shared_ptr<Azure::Storage::StorageSharedKeyCredential>(other.emulatedStorageCredential);
		}
		else
		{
			new(&this->cloudStorageCredential) std::shared_ptr<Azure::Core::Credentials::TokenCredential>(other.cloudStorageCredential);
		}
	}

	ServiceRequest::~ServiceRequest() {}

	Driver::Driver():
		bIsConnected(false)
	{
	}

	const string& Driver::GetName() const
	{
		return sName;
	}

	const string& Driver::GetVersion() const
	{
		return sVersion;
	}

	const string& Driver::GetScheme() const
	{
		return sScheme;
	}

	bool Driver::IsReadOnly() const
	{
		return false;
	}

	size_t Driver::GetPreferredBufferSize() const
	{
		return nPreferredBufferSize;
	}

	void Driver::Connect()
	{
		bIsConnected = true;
	}

	void Driver::Disconnect()
	{
		CheckConnected();
		bIsConnected = false;
	}

	bool Driver::IsConnected() const
	{
		return bIsConnected;
	}

	bool Driver::Exists(const string& sUrl) const
	{
		CheckConnected();
		ServiceRequest request = ParseUrl(sUrl);
		if (request.storageType == BLOB)
		{
			if (request.bDir)
			{
				return true; // there is no such concept as a directory when dealing with blob services
			}
			else
			{
				return !ListBlobs(request).empty();
			}
		}
		else // SHARE
		{
			if (request.bDir)
			{
				return !ListDirs(request).empty();
			}
			else
			{
				return !ListFiles(request).empty();
			}
		}
	}

	size_t Driver::GetSize(const string& sUrl) const
	{
		CheckConnected();
		ServiceRequest request = ParseUrl(sUrl);
		if (request.storageType == BLOB)
		{
			if (request.bDir)
			{
				throw InvalidOperationForDirError(DirOperation::GET_SIZE);
			}
			else
			{
				auto blobs = ListBlobs(request);
				if (blobs.empty())
				{
					throw NoFileError(sUrl);
				}
				return FileInfo(move(blobs)).GetSize();
			}
		}
		else // SHARE
		{
			if (request.bDir)
			{
				throw InvalidOperationForDirError(DirOperation::GET_SIZE);
			}
			else
			{
				auto files = ListFiles(request);
				if (files.empty())
				{
					throw NoFileError(sUrl);
				}
				return FileInfo(move(files)).GetSize();
			}
		}
	}

	FileReader& Driver::OpenForReading(const string& sUrl)
	{
		CheckConnected();
		ServiceRequest request = ParseUrl(sUrl);
		if (request.storageType == BLOB)
		{
			if (request.bDir)
			{
				throw InvalidOperationForDirError(DirOperation::READ);
			}
			else
			{
				auto blobs = ListBlobs(request);
				if (blobs.empty())
				{
					throw NoFileError(sUrl);
				}
				return RegisterReader(move(FileReader(move(blobs))));
			}
		}
		else // SHARE
		{
			if (request.bDir)
			{
				throw InvalidOperationForDirError(DirOperation::READ);
			}
			else
			{
				auto files = ListFiles(request);
				if (files.empty())
				{
					throw NoFileError(sUrl);
				}
				return RegisterReader(move(FileReader(move(files))));
			}
		}
	}

	FileWriter& Driver::OpenForWriting(const string& sUrl)
	{
		CheckConnected();
		ServiceRequest request = ParseUrl(sUrl);
		if (request.storageType == BLOB)
		{
			if (request.bDir)
			{
				throw InvalidOperationForDirError(DirOperation::WRITE);
			}
			else
			{
				return RegisterWriter(move(FileWriter(FileOutputMode::WRITE, move(GetBlobClient(request)))));
			}
		}
		else // SHARE
		{
			if (request.bDir)
			{
				throw InvalidOperationForDirError(DirOperation::WRITE);
			}
			else
			{
				CheckParentDirExists(request);
				return RegisterWriter(move(FileWriter(FileOutputMode::WRITE, move(GetFileClient(request)))));
			}
		}
	}
	
	FileWriter& Driver::OpenForAppending(const string& sUrl)
	{
		CheckConnected();
		ServiceRequest request = ParseUrl(sUrl);
		if (request.storageType == BLOB)
		{
			if (request.bDir)
			{
				throw InvalidOperationForDirError(DirOperation::APPEND);
			}
			else
			{
				return RegisterWriter(move(FileWriter(FileOutputMode::APPEND, move(GetBlobClient(request)))));
			}
		}
		else // SHARE
		{
			if (request.bDir)
			{
				throw InvalidOperationForDirError(DirOperation::APPEND);
			}
			else
			{
				string sFilename = request.share.path.back();
				Azure::Storage::Files::Shares::ListFilesAndDirectoriesOptions opts;
				opts.Prefix = sFilename;
				bool bAlreadyExisting = false;
				for (auto pagedResponse = GetParentDir(request).ListFilesAndDirectories(opts); pagedResponse.HasPage(); pagedResponse.MoveToNextPage())
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
				auto client = GetFileClient(request);
				if (!bAlreadyExisting)
				{
					client.Create(0);
				}
				return RegisterWriter(move(FileWriter(FileOutputMode::APPEND, move(client))));
			}
		}
	}

	void Driver::Close(void* handle)
	{
		RetrieveFileStream(handle, FileStreamType::ALL).Close();
	}
	
	size_t Driver::Read(void* handle, void* dest, size_t nSize, size_t nCount)
	{
		return ((FileReader&)RetrieveFileStream(handle, FileStreamType::READER)).Read(dest, nSize, nCount);
	}
	
	void Driver::Seek(void* handle, long long int nOffset, int nOrigin)
	{
		((FileReader&)RetrieveFileStream(handle, FileStreamType::READER)).Seek(nOffset, nOrigin);
	}

	size_t Driver::Write(void* handle, const void* source, size_t nSize, size_t nCount)
	{
		return ((FileWriter&)RetrieveFileStream(handle, FileStreamType::WRITER)).Write(source, nSize, nCount);
	}

	void Driver::Flush(void* handle)
	{
		((FileWriter&)RetrieveFileStream(handle, FileStreamType::WRITER)).Flush();
	}
	
	void Driver::Remove(const string& sUrl) const
	{
		CheckConnected();
		ServiceRequest request = ParseUrl(sUrl);
		if (request.storageType == BLOB)
		{
			if (request.bDir)
			{
				throw InvalidOperationForDirError(DirOperation::REMOVE);
			}
			else
			{
				auto blobs = ListBlobs(request);
				if (blobs.empty())
				{
					throw NoFileError(sUrl);
				}
				Azure::Storage::Blobs::DeleteBlobOptions opts;
				opts.DeleteSnapshots = Azure::Storage::Blobs::Models::DeleteSnapshotsOption::IncludeSnapshots;
				for (const auto& blob : blobs)
				{
					const string sBlobUrl = blob.GetUrl();
					if (!blob.Delete(opts).Value.Deleted)
					{
						throw DeletionError(sBlobUrl);
					}
				}
			}
		}
		else // SHARE
		{
			if (request.bDir)
			{
				throw InvalidOperationForDirError(DirOperation::REMOVE);
			}
			else
			{
				auto files = ListFiles(request);
				if (files.empty())
				{
					throw NoFileError(sUrl);
				}
				for (const auto& file : files)
				{
					const string sFileUrl = file.GetUrl();
					if (!file.Delete().Value.Deleted)
					{
						throw DeletionError(sFileUrl);
					}
				}
			}
		}
	}
	
	void Driver::MkDir(const string& sUrl) const
	{
		CheckConnected();
		ServiceRequest request = ParseUrl(sUrl);
		if (request.storageType == BLOB)
		{
			if (request.bDir)
			{
				// Do nothing
			}
			else
			{
				throw InvalidOperationForFileError(FileOperation::MKDIR);
			}
		}
		else // SHARE
		{
			if (request.bDir)
			{
				string sNewDir = request.share.path.back();
				auto parentDirClient = GetParentDir(request);

				Azure::Storage::Files::Shares::ListFilesAndDirectoriesOptions opts;
				opts.Prefix = sNewDir;
				for (auto pagedResponse = parentDirClient.ListFilesAndDirectories(opts); pagedResponse.HasPage(); pagedResponse.MoveToNextPage())
				{
					if (find_if(pagedResponse.Directories.begin(), pagedResponse.Directories.end(), [sNewDir](const auto& dirItem)
						{
							return dirItem.Name == sNewDir;
						}) != pagedResponse.Directories.end()
							)
					{
						throw DirAlreadyExistsError(sUrl);
					}
				}

				if (!parentDirClient.GetSubdirectoryClient(sNewDir).Create().Value.Created)
				{
					throw CreationError(sUrl);
				}
			}
			else
			{
				throw InvalidOperationForFileError(FileOperation::MKDIR);
			}
		}
	}
	
	void Driver::RmDir(const string& sUrl) const
	{
		CheckConnected();
		ServiceRequest request = ParseUrl(sUrl);
		if (request.storageType == BLOB)
		{
			if (request.bDir)
			{
				// Do nothing
			}
			else
			{
				throw InvalidOperationForFileError(FileOperation::RMDIR);
			}
		}
		else // SHARE
		{
			if (request.bDir)
			{
				auto dirs = ListDirs(request);
				if (dirs.empty())
				{
					throw NoFileError(sUrl);
				}
				for (const auto& dir : dirs)
				{
					const string sDirUrl = dir.GetUrl();
					if (!dir.Delete().Value.Deleted)
					{
						throw DeletionError(sDirUrl);
					}
				}
			}
			else
			{
				throw InvalidOperationForFileError(FileOperation::RMDIR);
			}
		}
	}
	
	size_t Driver::GetFreeDiskSpace() const
	{
		CheckConnected();
		return 5ULL * 1024ULL * 1024ULL * 1024ULL * 1024ULL;
	}
	
	void Driver::CopyTo(const string& sUrl, const std::string& destUrl)
	{
		CheckConnected();
		ServiceRequest request = ParseUrl(sUrl);
		if (request.storageType == BLOB)
		{
			if (request.bDir)
			{
				throw InvalidOperationForDirError(DirOperation::COPY);
			}
			else
			{
				auto& reader = OpenForReading(sUrl);
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
		}
		else // SHARE
		{
			if (request.bDir)
			{
				throw InvalidOperationForDirError(DirOperation::COPY);
			}
			else
			{
				auto& reader = OpenForReading(sUrl);
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
		}
	}
	
	void Driver::CopyFrom(const string& sUrl, const std::string& sourceUrl)
	{
		CheckConnected();
		ServiceRequest request = ParseUrl(sUrl);
		if (request.storageType == BLOB)
		{
			if (request.bDir)
			{
				throw InvalidOperationForDirError(DirOperation::COPY);
			}
			else
			{
				auto& writer = OpenForWriting(sUrl);
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
		}
		else // SHARE
		{
			if (request.bDir)
			{
				throw InvalidOperationForDirError(DirOperation::COPY);
			}
			else
			{
				auto& writer = OpenForWriting(sUrl);
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
		}
	}

	void Driver::CheckConnected() const
	{
		if (!IsConnected())
		{
			throw NotConnectedError();
		}
	}

	bool Driver::IsEmulatedStorage() const
	{
		return util::str::ToLower(util::env::GetEnvironmentVariableOrDefault("AZURE_EMULATED_STORAGE", "false")) != "false";
	}

	ServiceRequest Driver::ParseUrl(const string& sUrl) const
	{
		const string sBlobDomain = ".blob.core.windows.net";
		const string sFileDomain = ".file.core.windows.net";
		Azure::Core::Url url;
		try
		{
			url = Azure::Core::Url(sUrl);
		}
		catch (const exception&)
		{
			throw InvalidUrlError(sUrl);
		}
		const string& sHost = url.GetHost();
		const string& sPath = url.GetPath();
		bool bDir = util::str::EndsWith(sPath, "/");
		if (IsEmulatedStorage()) // Can only be a blob storage as only blob storages are supported by the emulator
		{
			smatch match;
			if (!regex_match(sPath, match, regex("([^/]+)/([^/]+)/(.+)"))) //  accountname/container/object or accountname/container/object/
			{
				throw InvalidObjectPathError(sPath);
			}
			auto connectionString = util::connstr::ConnectionString::ParseConnectionString(util::env::GetEnvironmentVariableOrThrow("AZURE_STORAGE_CONNECTION_STRING"));
			if (util::str::StartsWith(url.GetAbsoluteUrl(), connectionString.blobEndpoint.GetAbsoluteUrl()))
			{
				throw IncompatibleConnectionStringError();
			}
			auto credential = make_shared<Azure::Storage::StorageSharedKeyCredential>(connectionString.sAccountName, connectionString.sAccountKey);
			return ServiceRequest(url, StorageType::BLOB, bDir, BlobInfo{ match[1].str(), match[2].str(), match[3].str() }, credential);
		}
		else if (util::str::EndsWith(sHost, sBlobDomain))
		{
			smatch match;
			if (!regex_match(sPath, match, regex("([^/]+)/(.+)"))) //  container/object  or  container/object/
			{
				throw InvalidObjectPathError(sPath);
			}
			auto credential = make_shared<Azure::Identity::ChainedTokenCredential>(
				Azure::Identity::ChainedTokenCredential::Sources
				{
					std::make_shared<Azure::Identity::EnvironmentCredential>(), // for Client ID + Client Secret or Certificate environment variables
					std::make_shared<Azure::Identity::WorkloadIdentityCredential>(),
					std::make_shared<Azure::Identity::ManagedIdentityCredential>(),
					std::make_shared<Azure::Identity::AzureCliCredential>()
				}
			);
			return ServiceRequest(url, StorageType::BLOB, bDir, BlobInfo{ string(), match[1].str(), match[2].str()}, credential);
		}
		else if (util::str::EndsWith(sHost, sFileDomain))
		{
			smatch match;
			if (!regex_match(sPath, match, regex("([^/]+)((?:/[^/]+)+/?)"))) //  share/path/to/a/file  or  share/path/to/a/dir/
			{
				throw InvalidObjectPathError(sPath);
			}
			vector<string> fileOrDirPath = util::str::Split(match[2].str(), '/', -1, true);
			auto credential = make_shared<Azure::Identity::ChainedTokenCredential>(
				Azure::Identity::ChainedTokenCredential::Sources
				{
					std::make_shared<Azure::Identity::EnvironmentCredential>(), // for Client ID + Client Secret or Certificate environment variables
					std::make_shared<Azure::Identity::WorkloadIdentityCredential>(),
					std::make_shared<Azure::Identity::ManagedIdentityCredential>(),
					std::make_shared<Azure::Identity::AzureCliCredential>()
				}
			);
			return ServiceRequest(url, StorageType::SHARE, bDir, ShareInfo{ match[1].str(), fileOrDirPath }, credential);
		}
		else
		{
			throw InvalidDomainError(sHost);
		}
	}

	string Driver::GetServiceUrl(const ServiceRequest& request) const
	{
		if (request.storageType == BLOB)
		{
			if (request.bEmulated)
			{
				return (ostringstream() << request.azureUrl.GetScheme() << "://" << request.azureUrl.GetHost() << ":" << request.azureUrl.GetPort() << "/" << request.blob.sAccountName).str();
			}
			else
			{
				return (ostringstream() << request.azureUrl.GetScheme() << "://" << request.azureUrl.GetHost() << ":" << request.azureUrl.GetPort()).str();
			}
		}
		else // SHARE
		{
			return (ostringstream() << request.azureUrl.GetScheme() << "://" << request.azureUrl.GetHost() << ":" << request.azureUrl.GetPort()).str();
		}
	}

	string Driver::GetBlobContainerUrl(const ServiceRequest& request) const
	{
		return (ostringstream() << GetServiceUrl(request) << "/" << request.blob.sContainer).str();
	}

	Azure::Storage::Blobs::BlobServiceClient Driver::GetBlobServiceClient(const ServiceRequest& request) const
	{
		if (request.bEmulated)
		{
			return Azure::Storage::Blobs::BlobServiceClient(GetServiceUrl(request), request.emulatedStorageCredential);
		}
		else
		{
			return Azure::Storage::Blobs::BlobServiceClient(GetServiceUrl(request), request.cloudStorageCredential);
		}
	}

	Azure::Storage::Blobs::BlobContainerClient Driver::GetBlobContainerClient(const ServiceRequest& request) const
	{
		if (request.bEmulated)
		{
			return Azure::Storage::Blobs::BlobContainerClient(GetBlobContainerUrl(request), request.emulatedStorageCredential);
		}
		else
		{
			return Azure::Storage::Blobs::BlobContainerClient(GetBlobContainerUrl(request), request.cloudStorageCredential);
		}
	}

	Azure::Storage::Blobs::BlobClient Driver::GetBlobClient(const ServiceRequest& request) const
	{
		if (request.bEmulated)
		{
			return Azure::Storage::Blobs::BlobClient(request.azureUrl.GetAbsoluteUrl(), request.emulatedStorageCredential);
		}
		else
		{
			return Azure::Storage::Blobs::BlobClient(request.azureUrl.GetAbsoluteUrl(), request.cloudStorageCredential);
		}
	}

	vector<Azure::Storage::Blobs::BlobClient> Driver::ListBlobs(const ServiceRequest& request) const
	{
		return ResolveBlobsSearchString(GetBlobContainerClient(request), request.blob.sBlob);
	}

	string Driver::GetFileShareUrl(const ServiceRequest& request) const
	{
		return (ostringstream() << GetServiceUrl(request) << "/" << request.share.sShare).str();
	}

	Azure::Storage::Files::Shares::ShareServiceClient Driver::GetFileShareServiceClient(const ServiceRequest& request) const
	{
		return Azure::Storage::Files::Shares::ShareServiceClient(GetServiceUrl(request), request.cloudStorageCredential);
	}

	Azure::Storage::Files::Shares::ShareClient Driver::GetShareClient(const ServiceRequest& request) const
	{
		Azure::Storage::Files::Shares::ShareClientOptions opts;
		opts.ShareTokenIntent = Azure::Storage::Files::Shares::Models::ShareTokenIntent::Backup;
		return Azure::Storage::Files::Shares::ShareClient(GetFileShareUrl(request), request.cloudStorageCredential, opts);
	}

	Azure::Storage::Files::Shares::ShareDirectoryClient Driver::GetDirClient(const ServiceRequest& request) const
	{
		return GetShareClient(request).GetRootDirectoryClient();
	}

	Azure::Storage::Files::Shares::ShareFileClient Driver::GetFileClient(const ServiceRequest& request) const
	{
		Azure::Storage::Files::Shares::ShareClientOptions opts;
		opts.ShareTokenIntent = Azure::Storage::Files::Shares::Models::ShareTokenIntent::Backup;
		return Azure::Storage::Files::Shares::ShareFileClient(request.azureUrl.GetAbsoluteUrl(), request.cloudStorageCredential, opts);
	}

	vector<Azure::Storage::Files::Shares::ShareDirectoryClient> Driver::ListDirs(const ServiceRequest& request) const
	{
		return ResolveDirsPathRecursively(GetDirClient(request), queue<string, deque<string>>(deque<string>(request.share.path.begin(), request.share.path.end())));
	}

	vector<Azure::Storage::Files::Shares::ShareFileClient> Driver::ListFiles(const ServiceRequest& request) const
	{
		return ResolveFilesPathRecursively(GetDirClient(request), queue<string, deque<string>>(deque<string>(request.share.path.begin(), request.share.path.end())));
	}

	Azure::Storage::Files::Shares::ShareDirectoryClient Driver::GetParentDir(const ServiceRequest& request) const
	{
		Azure::Storage::Files::Shares::ShareDirectoryClient dirClient = GetDirClient(request);
		vector<string> path = request.share.path;
		path.pop_back();

		for (string sPathFragment : path)
		{
			Azure::Storage::Files::Shares::ListFilesAndDirectoriesOptions opts;
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

	void Driver::CheckParentDirExists(const ServiceRequest& request) const
	{
		GetParentDir(request);
	}

	FileReader& Driver::RegisterReader(FileReader&& reader)
	{
		void* handle = reader.GetHandle();
		fileReaders[handle] = make_unique<FileReader>(move(reader));
		return *fileReaders.at(handle);
	}

	FileWriter& Driver::RegisterWriter(FileWriter&& writer)
	{
		void* handle = writer.GetHandle();
		fileWriters[handle] = make_unique<FileWriter>(move(writer));
		return *fileWriters.at(handle);
	}

	FileStream& Driver::RetrieveFileStream(void* handle, FileStreamType streamType) const
	{
		if ((streamType & FileStreamType::READER) != FileStreamType::NONE)
		{
			auto rIt = fileReaders.find(handle);
			if (rIt != fileReaders.end())
			{
				return (FileStream&)*fileReaders.at(handle);
			}
		}
		if ((streamType & FileStreamType::WRITER) != FileStreamType::NONE)
		{
			auto wIt = fileWriters.find(handle);
			if (wIt != fileWriters.end())
			{
				return (FileStream&)*fileWriters.at(handle);
			}
		}
		throw FileStreamNotFoundError(handle);
	}
}
