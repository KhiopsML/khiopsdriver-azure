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
	class CloudFileAccessor
	{
	public:
		virtual ~CloudFileAccessor() = 0;

	protected:
		CloudFileAccessor();

		std::shared_ptr<Azure::Core::Credentials::TokenCredential> GetCredential() const;

	private:
		std::shared_ptr<Azure::Core::Credentials::TokenCredential> credential;

		static std::shared_ptr<Azure::Core::Credentials::TokenCredential> BuildCredential();
	};
}
