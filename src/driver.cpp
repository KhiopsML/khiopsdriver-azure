#include "driver.hpp"
#include <spdlog/spdlog.h>
#include <azure/core.hpp>
#include "util.hpp"
#include "cloudblobaccessor.hpp"
#include "cloudshareaccessor.hpp"
#include "emulatedblobaccessor.hpp"
#include "exception.hpp"

using namespace std;

namespace az
{
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

	unique_ptr<FileAccessor> Driver::CreateFileAccessor(const string& sUrl)
	{
		CheckConnected();
		const string sBlobDomain = ".blob.core.windows.net";
		const string sFileDomain = ".file.core.windows.net";
		Azure::Core::Url url;
		try
		{
			url = Azure::Core::Url(sUrl);
		}
		catch (const exception& exc)
		{
			throw InvalidUrlError(sUrl);
		}
		const string& sHost = url.GetHost();
		if (IsEmulatedStorage())
		{
			return make_unique<EmulatedBlobAccessor>(url, bind(&Driver::RegisterReader, this, placeholders::_1), bind(&Driver::RegisterWriter, this, placeholders::_1));
		}
		else if (util::str::EndsWith(sHost, sBlobDomain))
		{
			return make_unique<CloudBlobAccessor>(url, bind(&Driver::RegisterReader, this, placeholders::_1), bind(&Driver::RegisterWriter, this, placeholders::_1));
		}
		else if (util::str::EndsWith(sHost, sFileDomain))
		{
			return make_unique<CloudShareAccessor>(url, bind(&Driver::RegisterReader, this, placeholders::_1), bind(&Driver::RegisterWriter, this, placeholders::_1));
		}
		else
		{
			throw InvalidDomainError(sHost);
		}
	}

	FileStream& Driver::RetrieveFileStream(void* handle) const
	{
		return RetrieveFileStream(handle, true, true);
	}

	FileReader& Driver::RetrieveFileReader(void* handle) const
	{
		return (FileReader&)RetrieveFileStream(handle, true, false);
	}

	FileOutputStream& Driver::RetrieveFileOutputStream(void* handle) const
	{
		return (FileOutputStream&)RetrieveFileStream(handle, false, true);
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

	FileReader& Driver::RegisterReader(FileReader&& reader)
	{
		void* handle = reader.GetHandle();
		fileReaders.insert({ handle, move(reader) });
		return fileReaders.at(handle);
	}

	FileOutputStream& Driver::RegisterWriter(FileOutputStream&& writer)
	{
		void* handle = writer.GetHandle();
		fileWriters.insert({ handle, move(writer) });
		return fileWriters.at(handle);
	}

	FileStream& Driver::RetrieveFileStream(void* handle, bool bSearchReaders, bool bSearchWriters) const
	{
		if (bSearchReaders)
		{
			auto rIt = fileReaders.find(handle);
			if (rIt != fileReaders.end())
			{
				return (FileStream&)fileReaders.at(handle);
			}
		}
		if (bSearchWriters)
		{
			auto wIt = fileWriters.find(handle);
			if (wIt != fileWriters.end())
			{
				return (FileStream&)fileWriters.at(handle);
			}
		}
		throw FileStreamNotFoundError(handle);
	}
}
