#include "driver.h"
#include <spdlog/spdlog.h>
#include <azure/core.hpp>
#include "util/string.h"

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
		spdlog::debug("Connecting");
		bIsConnected = true;
	}

	void Driver::Disconnect()
	{
		spdlog::debug("Disconnecting");
		bIsConnected = false;
	}

	bool Driver::IsConnected() const
	{
		return bIsConnected;
	}

	FileAccessor& Driver::CreateFileAccessor(const string& sUrl) const
	{
		const string sBlobDomain = ".blob.core.windows.net";
		const string sFileDomain = ".file.core.windows.net";
		const Azure::Core::Url url(sUrl);
		const string& sHost = url.GetHost();
		if (EndsWith(sHost, sBlobDomain))
		{

		}
		else if (EndsWith(sHost, sFileDomain))
		{

		}
		else
		{

		}
	}
}
