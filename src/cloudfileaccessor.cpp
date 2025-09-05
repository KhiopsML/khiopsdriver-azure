#include "cloudfileaccessor.hpp"
#include <azure/identity/chained_token_credential.hpp>
#include <azure/identity/environment_credential.hpp>
#include <azure/identity/workload_identity_credential.hpp>
#include <azure/identity/azure_cli_credential.hpp>
#include <azure/identity/managed_identity_credential.hpp>
#include <iostream>

using namespace std;

namespace az
{
	CloudFileAccessor::~CloudFileAccessor()
	{
	}

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
		cout << "Tenant ID: " << (getenv("AZURE_TENANT_ID") ? getenv("AZURE_TENANT_ID") : "not set") << endl;
		cout << "Subscription ID: " << (getenv("AZURE_SUBSCRIPTION_ID") ? getenv("AZURE_SUBSCRIPTION_ID") : "not set") << endl;
		cout << "Client ID: " << (getenv("AZURE_CLIENT_ID") ? getenv("AZURE_CLIENT_ID") : "not set") << endl;
		cout << "Token available: " << (getenv("AZURE_TOKEN") ? "yes" : "no") << endl;

		// Redefining DefaultAzureCredential chain which does not work.
		// Chain schema: https://learn.microsoft.com/en-us/azure/developer/cpp/sdk/authentication/credential-chains#defaultazurecredential-overview.
		return make_shared<Azure::Identity::ChainedTokenCredential>(
			Azure::Identity::ChainedTokenCredential::Sources
			{
				make_shared<Azure::Identity::EnvironmentCredential>(), // for Client ID + Client Secret or Certificate environment variables
				make_shared<Azure::Identity::WorkloadIdentityCredential>(),
				make_shared<Azure::Identity::ManagedIdentityCredential>(),
				make_shared<Azure::Identity::AzureCliCredential>()
			}
		);
	}
}
