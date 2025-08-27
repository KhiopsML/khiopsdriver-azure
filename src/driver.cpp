#include "driver.hpp"
#include <spdlog/spdlog.h>
#include <azure/core.hpp>
#include "util/string.hpp"
#include "cloudblobaccessor.hpp"
#include "cloudshareaccessor.hpp"
#include "emulatedblobaccessor.hpp"
#include "util/env.hpp"
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
		return sPreferredBufferSize;
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
			return make_unique<EmulatedBlobAccessor>(url, bind(&Driver::RegisterReader, this, placeholders::_1), bind(&Driver::RegisterWriter, this, placeholders::_1), bind(&Driver::RegisterAppender, this, placeholders::_1));
		}
		else if (EndsWith(sHost, sBlobDomain))
		{
			return make_unique<CloudBlobAccessor>(url, bind(&Driver::RegisterReader, this, placeholders::_1), bind(&Driver::RegisterWriter, this, placeholders::_1), bind(&Driver::RegisterAppender, this, placeholders::_1));
		}
		else if (EndsWith(sHost, sFileDomain))
		{
			return make_unique<CloudShareAccessor>(url, bind(&Driver::RegisterReader, this, placeholders::_1), bind(&Driver::RegisterWriter, this, placeholders::_1), bind(&Driver::RegisterAppender, this, placeholders::_1));
		}
		else
		{
			throw InvalidDomainError(sHost);
		}
	}

	const unique_ptr<FileStream>& Driver::RetrieveFileStream(const FileStreamHandle& handle) const
	{
		return RetrieveFileStream(handle, true, true, true);
	}

	const unique_ptr<FileReader>& Driver::RetrieveFileReader(const FileStreamHandle& handle) const
	{
		return (const unique_ptr<FileReader>&)RetrieveFileStream(handle, true, false, false);
	}

	const unique_ptr<FileOutputStream>& Driver::RetrieveFileOutputStream(const FileStreamHandle& handle) const
	{
		return (const unique_ptr<FileOutputStream>&)RetrieveFileStream(handle, false, true, true);
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
		return ToLower(GetEnvironmentVariableOrDefault("AZURE_EMULATED_STORAGE", "false")) != "false";
	}

	const unique_ptr<FileReader>& Driver::RegisterReader(unique_ptr<FileReader> readerPtr)
	{
		return fileReaders[readerPtr->GetHandle()] = move(readerPtr);
	}

	const unique_ptr<FileWriter>& Driver::RegisterWriter(unique_ptr<FileWriter> writerPtr)
	{
		return fileWriters[writerPtr->GetHandle()] = move(writerPtr);
	}

	const unique_ptr<FileAppender>& Driver::RegisterAppender(unique_ptr<FileAppender> appenderPtr)
	{
		return fileAppenders[appenderPtr->GetHandle()] = move(appenderPtr);
	}

	const unique_ptr<FileStream>& Driver::RetrieveFileStream(const FileStreamHandle& handle, bool bSearchReaders, bool bSearchWriters, bool bSearchAppenders) const
	{
		if (bSearchReaders)
		{
			auto rIt = fileReaders.find(handle);
			if (rIt != fileReaders.end())
			{
				return (const unique_ptr<FileStream>&)rIt->second;
			}
		}
		if (bSearchWriters)
		{
			auto wIt = fileWriters.find(handle);
			if (wIt != fileWriters.end())
			{
				return (const unique_ptr<FileStream>&)wIt->second;
			}
		}
		if (bSearchAppenders)
		{
			auto aIt = fileAppenders.find(handle);
			if (aIt != fileAppenders.end())
			{
				return (const unique_ptr<FileStream>&)aIt->second;
			}
		}
		throw FileStreamNotFoundError(handle);
	}
}
