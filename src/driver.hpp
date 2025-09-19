#pragma once

namespace az
{
	struct BlobInfo;
	struct ShareInfo;
	struct ServiceRequest;
	class Driver;
}

#include <string>
#include <memory>
#include <unordered_map>
#include <azure/storage/blobs/blob_service_client.hpp>
#include <azure/storage/blobs/blob_container_client.hpp>
#include <azure/storage/blobs/blob_client.hpp>
#include <azure/storage/files/shares/share_service_client.hpp>
#include <azure/storage/files/shares/share_client.hpp>
#include <azure/storage/files/shares/share_directory_client.hpp>
#include <azure/storage/files/shares/share_file_client.hpp>
#include "filestream.hpp"
#include "macro.hpp"

// Release versions must have 3 digits, for example STRINGIFY(1.2.0)
// Alpha, beta ou release candidates have an extra suffix, for example :
// - STRINGIFY(1.2.0-a.1)
// - STRINGIFY(1.2.0-b.3)
// - STRINGIFY(1.2.0-rc.2)
#define DRIVER_VERSION STRINGIFY(0.1.0)

namespace az
{
	static const std::string sName = "Azure driver";
	static const std::string sVersion = DRIVER_VERSION;
	static const std::string sScheme = "https";
	static constexpr size_t nDefaultPreferredBufferSize = 4 * 1024 * 1024;

	struct BlobInfo
	{
		std::string sAccountName;
		std::string sContainer;
		std::string sBlob;
	};

	struct ShareInfo
	{
		std::string sShare;
		std::vector<std::string> path;
	};

	struct ServiceRequest
	{
		Azure::Core::Url azureUrl;
		StorageType storageType;
		bool bEmulated;
		bool bDir;
		union
		{
			BlobInfo blob;
			ShareInfo share;
		};
		union
		{
			std::shared_ptr<Azure::Storage::StorageSharedKeyCredential> emulatedStorageCredential;
			std::shared_ptr<Azure::Core::Credentials::TokenCredential> cloudStorageCredential;
		};
		ServiceRequest(const Azure::Core::Url azureUrl, StorageType storageType, bool bDir, const BlobInfo& blob, std::shared_ptr<Azure::Storage::StorageSharedKeyCredential> emulatedStorageCredential);
		ServiceRequest(const Azure::Core::Url azureUrl, StorageType storageType, bool bDir, const BlobInfo& blob, std::shared_ptr<Azure::Core::Credentials::TokenCredential> cloudStorageCredential);
		ServiceRequest(const Azure::Core::Url azureUrl, StorageType storageType, bool bDir, const ShareInfo& share, std::shared_ptr<Azure::Storage::StorageSharedKeyCredential> emulatedStorageCredential);
		ServiceRequest(const Azure::Core::Url azureUrl, StorageType storageType, bool bDir, const ShareInfo& share, std::shared_ptr<Azure::Core::Credentials::TokenCredential> cloudStorageCredential);
		ServiceRequest(const ServiceRequest& other);
		~ServiceRequest();
	};

	class Driver
	{
	public:
		Driver();
		~Driver();

		const std::string& GetName() const;
		const std::string& GetVersion() const;
		const std::string& GetScheme() const;
		bool IsReadOnly() const;
		size_t GetPreferredBufferSize() const;

		void Connect();
		void Disconnect();
		bool IsConnected() const;

		bool Exists(const std::string& sUrl) const;
		size_t GetSize(const std::string& sUrl) const;
		FileStream& OpenForReading(const std::string& sUrl);
		FileStream& OpenForWriting(const std::string& sUrl);
		FileStream& OpenForAppending(const std::string& sUrl);
		void Close(void* handle);
		size_t Read(void* handle, void* dest, size_t nSize, size_t nCount);
		void Seek(void* handle, long long int nOffset, int nOrigin);
		size_t Write(void* handle, const void* source, size_t nSize, size_t nCount);
		void Flush(void* handle);
		void Remove(const std::string& sUrl) const;
		void MkDir(const std::string& sUrl) const;
		void RmDir(const std::string& sUrl) const;
		size_t GetFreeDiskSpace() const;
		void CopyTo(const std::string& sUrl, const std::string& destUrl);
		void CopyFrom(const std::string& sUrl, const std::string& sourceUrl);
		void Concatenate(const std::vector<std::string>& inputUrls, const std::string& sDestUrl);

	private:
		void CheckConnected() const;
		bool IsEmulatedStorage() const;

		ServiceRequest ParseUrl(const std::string& sUrl) const;

		std::string GetServiceUrl(const ServiceRequest& request) const;
		std::string GetBlobContainerUrl(const ServiceRequest& request) const;
		Azure::Storage::Blobs::BlobServiceClient GetBlobServiceClient(const ServiceRequest& request) const;
		Azure::Storage::Blobs::BlobContainerClient GetBlobContainerClient(const ServiceRequest& request) const;
		Azure::Storage::Blobs::BlobClient GetBlobClient(const ServiceRequest& request) const;
		std::vector<Azure::Storage::Blobs::BlobClient> ListBlobs(const ServiceRequest& request) const;

		std::string GetFileShareUrl(const ServiceRequest& request) const;
		Azure::Storage::Files::Shares::ShareServiceClient GetFileShareServiceClient(const ServiceRequest& request) const;
		Azure::Storage::Files::Shares::ShareClient GetShareClient(const ServiceRequest& request) const;
		Azure::Storage::Files::Shares::ShareDirectoryClient GetDirClient(const ServiceRequest& request) const;
		Azure::Storage::Files::Shares::ShareFileClient GetFileClient(const ServiceRequest& request) const;
		std::vector<Azure::Storage::Files::Shares::ShareDirectoryClient> ListDirs(const ServiceRequest& request) const;
		std::vector<Azure::Storage::Files::Shares::ShareFileClient> ListFiles(const ServiceRequest& request) const;
		Azure::Storage::Files::Shares::ShareDirectoryClient GetParentDir(const ServiceRequest& request) const;
		void CheckParentDirExists(const ServiceRequest& request) const;

		FileStream& RegisterFileStream(FileStream&& fileStream);
		FileStream& RetrieveFileStream(void* handle) const;

		bool bIsConnected;

		size_t nPreferredBufferSize;

		std::unordered_map<void*, std::unique_ptr<FileStream>> fileStreams;
	};
}
