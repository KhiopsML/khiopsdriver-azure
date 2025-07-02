#pragma once

namespace az
{
	class EmulatedFileAccessor;
}

#include <string>
#include <azure/core/url.hpp>
#include "util/connstring.hpp"

namespace az
{
	using namespace std;

	class EmulatedFileAccessor
	{
	public:
		virtual ~EmulatedFileAccessor() = 0;

	protected:
		EmulatedFileAccessor();

		const ConnectionString& GetConnectionString() const;

		bool IsConnectionStringCompatibleWithUrl(const Azure::Core::Url& url) const;

	private:
		ConnectionString connectionString;

		static ConnectionString GetConnectionStringFromEnv();
	};
}
