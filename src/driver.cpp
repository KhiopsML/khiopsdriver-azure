#include "driver.hpp"
#include <spdlog/spdlog.h>
#include <azure/core.hpp>
#include "util/string.hpp"
#include "blobaccessor.hpp"
#include "shareaccessor.hpp"
#include "emulatedblobaccessor.hpp"
#include "exception.hpp"
#include "util/env.hpp"

namespace az
{
	Driver::Driver():
		bIsConnected(false),
		bIsEmulatedStorage(ToLower(GetEnvironmentVariableOrDefault("AZURE_EMULATED_STORAGE", "false")) != "false")
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
		const Azure::Core::Url url(sUrl);
		const string& sHost = url.GetHost();
		if (IsEmulatedStorage())
		{
			return make_unique<EmulatedBlobAccessor>(url);
		}
		else
		{
			if (EndsWith(sHost, sBlobDomain))
			{
				return make_unique<BlobAccessor>(url);
			}
			else if (EndsWith(sHost, sFileDomain))
			{
				return make_unique<ShareAccessor>(url);
			}
			else
			{
				throw InvalidDomainError();
			}
		}
	}

	FileStream Driver::RetrieveFileStream(void* handle) const
	{
		// TODO: Implement
		return FileStream();
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
		return bIsEmulatedStorage;
	}
}
