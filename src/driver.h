#pragma once

namespace az
{
	class Driver;
}

#include <string>
#include "fileaccessor.h"
#include "filestream.h"
#include "util/macro.h"

// Release versions must have 3 digits, for example KHIOPS_STR(1.2.0)
// Alpha, beta ou release candidates have an extra suffix, for example :
// - KHIOPS_STR(1.2.0-a.1)
// - KHIOPS_STR(1.2.0-b.3)
// - KHIOPS_STR(1.2.0-rc.2)
#define DRIVER_VERSION KHIOPS_STR(0.1.0)

namespace az
{
	using namespace std;

	static const string sName = "Azure driver";
	static const string sVersion = DRIVER_VERSION;
	static const string sScheme = "https";
	static const size_t sPreferredBufferSize = 4 * 1024 * 1024;

	class Driver
	{
	public:
		Driver();

		const string& GetName() const;
		const string& GetVersion() const;
		const string& GetScheme() const;
		bool IsReadOnly() const;
		size_t GetPreferredBufferSize() const;

		void Connect();
		void Disconnect();
		inline bool IsConnected() const;

		FileAccessor&& CreateFileAccessor(const string& url) const;
		FileStream RetrieveFileStream(void* handle) const;

	protected:
		void CheckConnected() const;

		bool bIsConnected;
	};
}
