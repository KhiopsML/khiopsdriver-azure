#pragma once

namespace az
{
	class Driver;
}

#include <string>
#include <memory>
#include <unordered_map>
#include "fileaccessor.hpp"
#include "filestream.hpp"
#include "util/macro.hpp"

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
	static const size_t nPreferredBufferSize = 4 * 1024 * 1024;

	class Driver
	{
	public:
		Driver();

		const std::string& GetName() const;
		const std::string& GetVersion() const;
		const std::string& GetScheme() const;
		bool IsReadOnly() const;
		size_t GetPreferredBufferSize() const;

		void Connect();
		void Disconnect();
		bool IsConnected() const;

		std::unique_ptr<FileAccessor> CreateFileAccessor(const std::string& url);
		FileStream& RetrieveFileStream(void* handle) const;
		FileReader& RetrieveFileReader(void* handle) const;
		FileOutputStream& RetrieveFileOutputStream(void* handle) const;

	protected:
		void CheckConnected() const;
		bool IsEmulatedStorage() const;

		FileReader& RegisterReader(FileReader&& reader);
		FileOutputStream& RegisterWriter(FileOutputStream&& writer);

		FileStream& RetrieveFileStream(void* handle, bool bSearchReaders, bool bSearchWriters) const;

		bool bIsConnected;

		std::unordered_map<void*, FileReader> fileReaders;
		std::unordered_map<void*, FileOutputStream> fileWriters;
	};
}
