#include "objectclient.hpp"

namespace az
{
	ObjectClient::ObjectClient(const ObjectClient& source) :
		tag(source.tag)
	{
		if (tag == StorageType::BLOB)
			new(&blob) Azure::Storage::Blobs::BlobClient(source.blob);
		else
			new(&shareFile) Azure::Storage::Files::Shares::ShareFileClient(source.shareFile);
	}
	ObjectClient::ObjectClient(const Azure::Storage::Blobs::BlobClient& client) : tag(StorageType::BLOB), blob(client) {}
	ObjectClient::ObjectClient(const Azure::Storage::Files::Shares::ShareFileClient& client) : tag(StorageType::SHARE), shareFile(client) {}
	ObjectClient::~ObjectClient() {}
	ObjectClient& ObjectClient::operator=(const ObjectClient& source)
	{
		tag = source.tag;
		if (tag == StorageType::BLOB)
			blob = source.blob;
		else
			shareFile = source.shareFile;
		return *this;
	}
}
