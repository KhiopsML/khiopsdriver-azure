#pragma once

namespace az
{
	enum class FileStreamType;
	class FileStream;
	class FileReader;
	enum class FileOutputMode;
	class FileWriter;
}

#include <cstddef>
#include <vector>
#include <string>
#include <azure/storage/blobs/blob_client.hpp>
#include <azure/storage/files/shares/share_file_client.hpp>
#include "filestream.hpp"
#include "storagetype.hpp"
#include "objectclient.hpp"
#include "fileinfo.hpp"

namespace az
{
	enum class FileStreamType
	{
		NONE = 0,
		READER = 1 << 0,
		WRITER = 1 << 1,
		ALL = READER | WRITER
	};

	FileStreamType operator&(const FileStreamType& a, const FileStreamType& b);

	class FileStream
	{
	public:
		virtual ~FileStream();
		void* GetHandle() const;
		virtual void Close() = 0;

	protected:
		FileStream();

	private:
		void* handle;
	};

	class FileReader : public FileStream
	{
	public:
		FileReader(std::vector<Azure::Storage::Blobs::BlobClient>&& clients);
		FileReader(std::vector<Azure::Storage::Files::Shares::ShareFileClient>&& clients);
		FileReader(std::vector<ObjectClient>&& clients);
		~FileReader();
		void Close() override;
		size_t Read(void* dest, size_t nSize, size_t nCount);
		void Seek(long long int nOffset, int nOrigin);

	private:
		StorageType storageType;
		FileInfo fileInfo;
		size_t nCurrentPos;
	};

	enum class FileOutputMode
	{
		WRITE,
		APPEND
	};

	class FileWriter : public FileStream
	{
	public:
		FileWriter(FileOutputMode mode, Azure::Storage::Blobs::BlobClient&& client);
		FileWriter(FileOutputMode mode, Azure::Storage::Files::Shares::ShareFileClient&& client);
		FileWriter(FileOutputMode mode, ObjectClient&& client);
		~FileWriter();
		void Close();
		size_t Write(const void* source, size_t nSize, size_t nCount);
		void Flush();

	private:
		StorageType storageType;
		FileOutputMode mode;
		ObjectClient client;
		size_t nCurrentPos;
		std::vector<std::string> blockIds;
	};
}
