#pragma once

namespace az
{
	class ErrorLogger;
}

#include <string>

using namespace std;

namespace az
{
	class ErrorLogger
	{
	public:
		ErrorLogger();

		const string& GetLastError() const;
		void LogError(const string& error);
		void LogNullArgError(const string& funcname, const string& argname);
		void LogException(const exception& exc);

	protected:
		string sLastError;
	};
}
