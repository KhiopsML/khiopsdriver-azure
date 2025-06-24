#pragma once

class Driver;

#include <string>

using namespace std;

class Driver
{
public:
	Driver();

	string GetName() const;
	string GetVersion() const;
	string GetScheme() const;
	bool IsReadOnly() const;
	size_t GetSystemPreferredBufferSize() const;

	void Connect();
	void Disconnect();
	bool IsConnected() const;

	CreateObjectAccessor();

protected:
	bool bIsConnected;
};
