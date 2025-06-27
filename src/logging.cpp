#include "logging.h"
#include <string>
#include <spdlog/spdlog.h>
#include "util/env.h"

using namespace std;

namespace az
{
	void ConfigureLogLevel()
	{
		const string loglevel = GetEnvironmentVariableOrDefault("AZURE_DRIVER_LOGLEVEL", "info");
		if (loglevel == "debug")
		{
			spdlog::set_level(spdlog::level::debug);
		}
		else if (loglevel == "trace")
		{
			spdlog::set_level(spdlog::level::trace);
		}
		else
		{
			spdlog::set_level(spdlog::level::info);
		}
	}
}
