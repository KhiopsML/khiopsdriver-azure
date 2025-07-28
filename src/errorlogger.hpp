#pragma once

namespace az
{
	class ErrorLogger;
}

#include <string>

namespace az
{
	class ErrorLogger
	{
	public:
		ErrorLogger();

		const std::string& GetLastError() const;
		void LogError(const std::string& error);
		void LogNullArgError(const std::string& funcname, const std::string& argname);
		void LogException(const exception& exc);

	protected:
		std::string sLastError;
	};
}
