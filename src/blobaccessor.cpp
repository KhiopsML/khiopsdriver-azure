#include "blobaccessor.hpp"
#include "exception.hpp"
#include "util/connstring.hpp"
#include "util/string.hpp"
#include "util/env.hpp"

namespace az
{
	BlobAccessor::BlobAccessor(const Azure::Core::Url& url, bool bIsEmulatedStorage):
		FileAccessor(url),
		bIsEmulatedStorage(ToLower(GetEnvironmentVariableOrDefault("AZURE_EMULATED_STORAGE", "false")) != "false")
	{
	}

	bool BlobAccessor::Exists() const
	{
		if (HasDirUrl())
		{
			return true;
		}
		else
		{
			try
			{
				GetBlobClient().GetProperties();
				return true;
			}
			catch (const exception& exc)
			{
				auto what = exc.what();//Azure::Core::Http::TransportException
				return false;
			}
		}
	}

	size_t BlobAccessor::GetSize() const
	{
		// TODO: Implement
		return 0;
	}

	FileStream BlobAccessor::Open(char mode) const
	{
		// TODO: Implement
		return FileStream();
	}

	void BlobAccessor::Remove() const
	{
		// TODO: Implement
	}

	void BlobAccessor::MkDir() const
	{
		// TODO: Implement
	}

	void BlobAccessor::RmDir() const
	{
		// TODO: Implement
	}

	size_t BlobAccessor::GetFreeDiskSpace() const
	{
		// TODO: Implement
		return 0;
	}

	void BlobAccessor::CopyTo(const Azure::Core::Url& destUrl) const
	{
		// TODO: Implement
	}

	void BlobAccessor::CopyFrom(const Azure::Core::Url& sourceUrl) const
	{
		// TODO: Implement
	}

	BlobAccessor::~BlobAccessor()
	{
	}

	bool BlobAccessor::IsEmulatedStorage() const
	{
		return bIsEmulatedStorage;
	}

	Azure::Storage::Blobs::BlobClient BlobAccessor::GetBlobClient() const
	{
		if (IsEmulatedStorage())
		{
			Account account = GetAccount();
			return Azure::Storage::Blobs::BlobClient(
				GetUrl().GetAbsoluteUrl(),
				make_shared<Azure::Storage::StorageSharedKeyCredential>(account.name, account.key)
			);
		}
		else
		{
			return Azure::Storage::Blobs::BlobClient(GetUrl().GetAbsoluteUrl(), GetCredential());
		}
	}
}
