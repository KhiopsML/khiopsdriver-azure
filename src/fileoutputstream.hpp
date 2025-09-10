#pragma once

namespace az
{
	enum class FileOutputMode;
	class FileOutputStream;
}

#include <cstddef>
#include <azure/storage/blobs/blob_client.hpp>
#include <azure/storage/files/shares/share_file_client.hpp>
#include "objectclient.hpp"
#include "filestream.hpp"
#include "storagetype.hpp"

namespace az
{
	enum class FileOutputMode
	{
		WRITE,
		APPEND
	};

	class UnknownFileOutputModeError : public Error
	{
	public:
		inline UnknownFileOutputModeError(FileOutputMode fileOutputMode) :
			sMessage((std::ostringstream() << "unknown file output mode '" << (int)fileOutputMode << "'").str())
		{
		}

		inline virtual const char* what() const noexcept override
		{
			return sMessage.c_str();
		}

	private:
		std::string sMessage;
	};

	class FileOutputStream : public FileStream
	{
	public:
		FileOutputStream(FileOutputMode mode, Azure::Storage::Blobs::BlobClient&& client);
		FileOutputStream(FileOutputMode mode, Azure::Storage::Files::Shares::ShareFileClient&& client);
		FileOutputStream(FileOutputMode mode, ObjectClient&& client);
		void Close();
		size_t Write(const void* source, size_t nSize, size_t nCount);
		void Flush();

	private:
		StorageType storageType;
		FileOutputMode mode;
		ObjectClient client;
		size_t nCurrentPos;
	};
}
