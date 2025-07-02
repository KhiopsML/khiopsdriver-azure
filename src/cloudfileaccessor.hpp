#pragma once

namespace az
{
	class CloudFileAccessor;
}

#include <memory>
#include <azure/core/credentials/credentials.hpp>
#include <azure/core/url.hpp>

namespace az
{
	using namespace std;

	class CloudFileAccessor
	{
	public:
		virtual ~CloudFileAccessor() = 0;

	protected:
		CloudFileAccessor();

		shared_ptr<Azure::Core::Credentials::TokenCredential> GetCredential() const;

	private:
		shared_ptr<Azure::Core::Credentials::TokenCredential> credential;

		static shared_ptr<Azure::Core::Credentials::TokenCredential> BuildCredential();
	};
}
