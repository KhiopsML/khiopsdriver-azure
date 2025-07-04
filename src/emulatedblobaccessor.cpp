#include "emulatedblobaccessor.hpp"
#include "util/connstring.hpp"
#include "util/string.hpp"

namespace az
{
	EmulatedBlobAccessor::EmulatedBlobAccessor(const Azure::Core::Url& url):
		BlobAccessor(url),
		EmulatedFileAccessor()
	{
		if (!IsConnectionStringCompatibleWithUrl(url))
		{
			throw IncompatibleConnectionStringError();
		}
	}

	EmulatedBlobAccessor::~EmulatedBlobAccessor()
	{
	}

	string EmulatedBlobAccessor::GetAccountName() const
	{
		return UrlPathParts().at(0);
	}

	string EmulatedBlobAccessor::GetContainerName() const
	{
		return UrlPathParts().at(1);
	}

	string EmulatedBlobAccessor::GetObjectName() const
	{
		return UrlPathParts().at(2);
	}

	string EmulatedBlobAccessor::GetServiceUrl() const
	{
		const auto& url = GetUrl();
		return (ostringstream() << url.GetScheme() << "://" << url.GetHost() << ":" << url.GetPort() << "/" << GetAccountName()).str();
	}

	string EmulatedBlobAccessor::GetContainerUrl() const
	{
		return (ostringstream() << GetServiceUrl() << "/" << GetContainerName()).str();
	}
	
	Azure::Storage::Blobs::BlobServiceClient EmulatedBlobAccessor::GetServiceClient() const
	{
		return Azure::Storage::Blobs::BlobServiceClient(GetServiceUrl(), GetCredential());
	}

	Azure::Storage::Blobs::BlobContainerClient EmulatedBlobAccessor::GetContainerClient() const
	{
		return Azure::Storage::Blobs::BlobContainerClient(GetContainerUrl(), GetCredential());
	}

	Azure::Storage::Blobs::BlobClient EmulatedBlobAccessor::GetBlobClient() const
	{
		return Azure::Storage::Blobs::BlobClient(GetUrl().GetAbsoluteUrl(), GetCredential());
	}

	vector<string> EmulatedBlobAccessor::UrlPathParts() const
	{
		return Split(GetUrl().GetPath(), '/', 2); // <account> / <container> / <object>
	}
}
