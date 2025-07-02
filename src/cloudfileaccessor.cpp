#include "cloudfileaccessor.hpp"
#include <azure/identity/chained_token_credential.hpp>
#include <azure/identity/environment_credential.hpp>
#include <azure/identity/workload_identity_credential.hpp>
#include <azure/identity/azure_cli_credential.hpp>
#include <azure/identity/managed_identity_credential.hpp>

namespace az
{
	CloudFileAccessor::CloudFileAccessor():
		credential(BuildCredential())
	{
	}

	shared_ptr<Azure::Core::Credentials::TokenCredential> CloudFileAccessor::GetCredential() const
	{
		return credential;
	}

	shared_ptr<Azure::Core::Credentials::TokenCredential> CloudFileAccessor::BuildCredential()
	{
		// Redefining DefaultAzureCredential chain which does not work.
		// Chain schema: https://learn.microsoft.com/en-us/azure/developer/cpp/sdk/authentication/credential-chains#defaultazurecredential-overview.
		return make_shared<Azure::Identity::ChainedTokenCredential>(
			Azure::Identity::ChainedTokenCredential::Sources
			{
				std::make_shared<Azure::Identity::EnvironmentCredential>(), // for Client ID + Client Secret or Certificate environment variables
				std::make_shared<Azure::Identity::WorkloadIdentityCredential>(),
				std::make_shared<Azure::Identity::AzureCliCredential>(),
				std::make_shared<Azure::Identity::ManagedIdentityCredential>()
			}
		);
	}
}
