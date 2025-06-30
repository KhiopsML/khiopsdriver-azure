#include "driver.h"
#include <spdlog/spdlog.h>
#include <azure/core.hpp>
#include "util/string.h"
#include "blobaccessor.h"
#include "shareaccessor.h"
#include "exception.h"

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

	inline bool Driver::IsConnected() const
	{
		return bIsConnected;
	}

	FileAccessor&& Driver::CreateFileAccessor(const string& sUrl) const
	{
		CheckConnected();
		const string sBlobDomain = ".blob.core.windows.net";
		const string sFileDomain = ".file.core.windows.net";
		const Azure::Core::Url url(sUrl);
		const string& sHost = url.GetHost();
		if (EndsWith(sHost, sBlobDomain))
		{
			return move(BlobAccessor(url));
		}
		else if (EndsWith(sHost, sFileDomain))
		{
			return move(ShareAccessor(url));
		}
		else
		{
			throw InvalidDomainException();
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
			throw NotConnectedException();
		}
	}
}
