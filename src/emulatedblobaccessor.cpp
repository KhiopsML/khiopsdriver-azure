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

	string EmulatedBlobAccessor::GetServiceUrl() const
	{
		const auto& url = GetUrl();
		return (ostringstream() << url.GetScheme() << "://" << url.GetHost() << ":" << url.GetPort() << "/" << UrlPathParts().at(0)).str();
	}

	string EmulatedBlobAccessor::GetContainerUrl() const
	{
		return (ostringstream() << GetServiceUrl() << "/" << UrlPathParts().at(1)).str();
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
		vector<string> parts = Split(GetUrl().GetPath(), '/', 2);
		parts.erase(parts.begin());
		return parts;
	}
}
