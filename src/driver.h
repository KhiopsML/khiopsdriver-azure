#pragma once

class Driver;

#include <string>

using namespace std;

	static const string sName = "Azure driver";
	static const string sVersion = DRIVER_VERSION;
	static const string sScheme = "https";
	static const size_t sPreferredBufferSize = 4 * 1024 * 1024;

class Driver
{
public:
	Driver();

	string GetName() const;
	string GetVersion() const;
	string GetScheme() const;
	bool IsReadOnly() const;
		size_t GetPreferredBufferSize() const;

	void Connect();
	void Disconnect();
	bool IsConnected() const;

		FileAccessor CreateFileAccessor(string url);
		FileStream RetrieveFileStream(void* handle) const;

protected:
	bool bIsConnected;
};
