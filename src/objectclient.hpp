// Tagged union abstracting the concepts of blob clients and share file clients
// to a single "object client".

#pragma once

#include "storagetype.hpp"
#include <azure/storage/blobs/blob_client.hpp>
#include <azure/storage/files/shares/share_file_client.hpp>

namespace az {
struct ObjectClient {
  StorageType tag;
  union {
    Azure::Storage::Blobs::BlobClient blob;
    Azure::Storage::Files::Shares::ShareFileClient shareFile;
  };
  ObjectClient(const ObjectClient &source) : tag(source.tag) {
    if (tag == BLOB)
      new (&blob) Azure::Storage::Blobs::BlobClient(source.blob);
    else
      new (&shareFile)
          Azure::Storage::Files::Shares::ShareFileClient(source.shareFile);
  }
  ObjectClient(const Azure::Storage::Blobs::BlobClient &client) : tag(BLOB) {
    new (&blob) Azure::Storage::Blobs::BlobClient(client);
  }
  ObjectClient(const Azure::Storage::Files::Shares::ShareFileClient &client)
      : tag(SHARE) {
    new (&shareFile) Azure::Storage::Files::Shares::ShareFileClient(client);
  }
  ~ObjectClient() {
    if (tag == BLOB)
      blob.~BlobClient();
    else
      shareFile.~ShareFileClient();
  }
  ObjectClient &operator=(const ObjectClient &source) {
    tag = source.tag;
    if (tag == BLOB)
      blob = source.blob;
    else
      shareFile = source.shareFile;
    return *this;
  }
};
} // namespace az
