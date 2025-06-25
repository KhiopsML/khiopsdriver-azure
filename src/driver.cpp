#include "driver.h"
#include <spdlog/spdlog.h>

namespace az
{
	Driver::Driver():
		bIsConnected(false)
	{
	}

	string Driver::GetName() const
	{
		return sName;
	}

	string Driver::GetVersion() const
	{
		return sVersion;
	}

	string Driver::GetScheme() const
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
}
