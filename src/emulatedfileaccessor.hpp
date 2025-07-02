#pragma once

namespace az
{
	class EmulatedFileAccessor;
}

#include <string>
#include <azure/core/url.hpp>

namespace az
{
	using namespace std;

	class EmulatedFileAccessor
	{
	public:
		virtual ~EmulatedFileAccessor() = 0;

	protected:
		EmulatedFileAccessor(const string& sConnectionString);

		const string& GetConnectionString() const;

		bool IsConnectionStringCompatibleWithUrl(const Azure::Core::Url& url) const;

	private:
		string sConnectionString;
	};
}
