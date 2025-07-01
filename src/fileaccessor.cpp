#include "fileaccessor.hpp"
#include <azure/identity/chained_token_credential.hpp>
#include <azure/identity/environment_credential.hpp>
#include <azure/identity/workload_identity_credential.hpp>
#include <azure/identity/azure_cli_credential.hpp>
#include <azure/identity/managed_identity_credential.hpp>
#include "util/string.hpp"
#include "util/env.hpp"
#include "exception.hpp"

namespace az
{
	const Azure::Core::Url& FileAccessor::GetUrl() const
	{
		return url;
	}

	bool FileAccessor::HasDirUrl() const
	{
		return bHasDirUrl;
	}

	shared_ptr<Azure::Core::Credentials::TokenCredential> FileAccessor::GetCredential() const
	{
		return credential;
	}

	FileAccessor::~FileAccessor()
	{
	}

	FileAccessor::FileAccessor(const Azure::Core::Url& url, bool bIsEmulatedStorage):
		url(url),
		bHasDirUrl(EndsWith(url.GetPath(), "/")),
		bIsEmulatedStorage(bIsEmulatedStorage),
		credential(BuildCredential())
	{
		if (IsEmulatedStorage())
		{
			sConnectionString = GetEnvironmentVariableOrThrow("AZURE_STORAGE_CONNECTION_STRING");
		}
	}

	bool FileAccessor::IsEmulatedStorage() const
	{
		return bIsEmulatedStorage;
	}

	const string& FileAccessor::GetConnectionString() const
	{
		return sConnectionString;
	}

	shared_ptr<Azure::Core::Credentials::TokenCredential> FileAccessor::BuildCredential()
	{
		// Redefining DefaultAzureCredential chain which does not work.
		// Chain schema: https://learn.microsoft.com/en-us/azure/developer/cpp/sdk/authentication/credential-chains#defaultazurecredential-overview.
		return make_shared<Azure::Identity::ChainedTokenCredential>(
			Azure::Identity::ChainedTokenCredential::Sources{
				std::make_shared<Azure::Identity::EnvironmentCredential>(), // for Client ID + Client Secret or Certificate environment variables
				std::make_shared<Azure::Identity::WorkloadIdentityCredential>(),
				std::make_shared<Azure::Identity::AzureCliCredential>(),
				std::make_shared<Azure::Identity::ManagedIdentityCredential>()
			}
		);
	}
}
