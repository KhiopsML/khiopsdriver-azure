#include "cloudblobaccessor.hpp"
#include <sstream>
#include <regex>
#include "exception.hpp"
#include "util/string.hpp"

using namespace std;

namespace az
{
	CloudBlobAccessor::CloudBlobAccessor(const Azure::Core::Url& url):
		BlobAccessor(url),
		CloudFileAccessor()
	{
		CheckUrl();
	}

	CloudBlobAccessor::~CloudBlobAccessor()
	{
	}

	string CloudBlobAccessor::GetContainerName() const
	{
		return UrlPathParts().at(0);
	}

	string CloudBlobAccessor::GetObjectName() const
	{
		return UrlPathParts().at(1);
	}

	string CloudBlobAccessor::GetServiceUrl() const
	{
		const auto& url = GetUrl();
		return (ostringstream() << url.GetScheme() << "://" << url.GetHost() << ":" << url.GetPort()).str();
	}

	string CloudBlobAccessor::GetContainerUrl() const
	{
		return (ostringstream() << GetServiceUrl() << "/" << GetContainerName()).str();
	}

	Azure::Storage::Blobs::BlobServiceClient CloudBlobAccessor::GetServiceClient() const
	{
		return Azure::Storage::Blobs::BlobServiceClient(GetServiceUrl(), GetCredential());
	}

	Azure::Storage::Blobs::BlobContainerClient CloudBlobAccessor::GetContainerClient() const
	{
		return Azure::Storage::Blobs::BlobContainerClient(GetContainerUrl(), GetCredential());
	}

	Azure::Storage::Blobs::BlobClient CloudBlobAccessor::GetBlobClient() const
	{
		return Azure::Storage::Blobs::BlobClient(GetUrl().GetAbsoluteUrl(), GetCredential());
	}

	vector<string> CloudBlobAccessor::UrlPathParts() const
	{
		return Split(GetUrl().GetPath(), '/', 1); // <container> / <object>
	}

	void CloudBlobAccessor::CheckFileUrl() const
	{
		const string& path = GetUrl().GetPath();
		if (!regex_match(path, regex("[^/]+(?:/[^/]+)+")))
		{
			throw InvalidFileUrlPathError(path);
		}
	}

	void CloudBlobAccessor::CheckDirUrl() const
	{
		const string& path = GetUrl().GetPath();
		if (!regex_match(path, regex("(?:[^/]+/){2,}")))
		{
			throw InvalidDirUrlPathError(path);
		}
	}
}
