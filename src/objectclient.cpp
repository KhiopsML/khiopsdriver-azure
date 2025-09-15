#include "objectclient.hpp"

namespace az
{
	ObjectClient::ObjectClient(const ObjectClient& source) :
		tag(source.tag)
	{
		if (tag == BLOB)
			new(&blob) Azure::Storage::Blobs::BlobClient(source.blob);
		else
			new(&shareFile) Azure::Storage::Files::Shares::ShareFileClient(source.shareFile);
	}
	ObjectClient::ObjectClient(const Azure::Storage::Blobs::BlobClient& client) : tag(BLOB), blob(client) {}
	ObjectClient::ObjectClient(const Azure::Storage::Files::Shares::ShareFileClient& client) : tag(SHARE), shareFile(client) {}
	ObjectClient::~ObjectClient() {}
	ObjectClient& ObjectClient::operator=(const ObjectClient& source)
	{
		tag = source.tag;
		if (tag == BLOB)
			blob = source.blob;
		else
			shareFile = source.shareFile;
		return *this;
	}
}
