#include "errorlogger.hpp"
#include <sstream>
#include "spdlog/spdlog.h"

namespace az
{
	ErrorLogger::ErrorLogger() :
		sLastError("")
	{
	}

	const string& ErrorLogger::GetLastError() const
	{
		return sLastError;
	}

	void ErrorLogger::LogError(const string& error)
	{
		spdlog::error(sLastError = error);
	}

	void ErrorLogger::LogNullArgError(const string& funcname, const string& argname)
	{
		LogError((ostringstream() << "Error passing null pointer as '" << argname << "' argument to function '" << funcname << "'").str());
	}

	void ErrorLogger::LogException(const exception& exc)
	{
		LogError(exc.what());
	}
}
