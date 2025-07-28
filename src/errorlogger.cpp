#include "errorlogger.hpp"
#include <sstream>
#include "spdlog/spdlog.h"

using namespace std;

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

	void ErrorLogger::LogException(const exception& exc)
	{
		LogError(exc.what());
	}
}
