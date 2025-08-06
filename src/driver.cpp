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

	unique_ptr<FileAccessor> Driver::CreateFileAccessor(const string& sUrl) const
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
			return make_unique<EmulatedBlobAccessor>(url);
		}
		else if (EndsWith(sHost, sBlobDomain))
		{
			return make_unique<CloudBlobAccessor>(url);
		}
		else if (EndsWith(sHost, sFileDomain))
		{
			return make_unique<CloudShareAccessor>(url);
		}
		else
		{
			throw InvalidDomainError(sHost);
		}
	}

	unique_ptr<FileReader> Driver::RetrieveFileReader(const FileStreamHandle& handle) const
	{
		// TODO: Implement
		return nullptr;
	}
#if false
	FileWriter Driver::RetrieveFileWriter(const FileStreamHandle& handle) const
	{

	}

	FileAppender Driver::RetrieveFileAppender(const FileStreamHandle& handle) const
	{

	}
#endif
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
}
