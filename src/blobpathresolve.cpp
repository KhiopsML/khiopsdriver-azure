#include "blobpathresolve.hpp"
#include "contrib.hpp"
#include "util.hpp"
#include <functional>

using namespace std;

namespace az {
using BlobClient = Azure::Storage::Blobs::BlobClient;
using BlobContainerClient = Azure::Storage::Blobs::BlobContainerClient;
using ListBlobsOptions = Azure::Storage::Blobs::ListBlobsOptions;
using BlobItem = Azure::Storage::Blobs::Models::BlobItem;

static vector<BlobClient>
FindBlobsByName(const BlobContainerClient &containerClient,
                const string &sName);

static vector<BlobClient>
FindBlobsByGlob(const BlobContainerClient &containerClient,
                const string &sGlob);

static vector<BlobClient>
FindBlobs(const BlobContainerClient &containerClient,
          const function<bool(const BlobItem &item)> &Predicate,
          const string &sPrefix);

static string PrefixFromName(const string &sName);

static string PrefixFromGlob(const string &sGlob);

vector<BlobClient>
ResolveBlobsSearchString(const BlobContainerClient &containerClient,
                         const string &sSearchString) {
  return util::glob::FindGlobbingChar(sSearchString) != string::npos
             ? FindBlobsByGlob(containerClient, sSearchString)
             : FindBlobsByName(containerClient, sSearchString);
}

static vector<BlobClient>
FindBlobsByName(const BlobContainerClient &containerClient,
                const string &sName) {
  return FindBlobs(
      containerClient,
      [sName](const BlobItem &item) {
        return !item.IsDeleted && item.Name == sName;
      },
      PrefixFromName(sName));
}

static vector<BlobClient>
FindBlobsByGlob(const BlobContainerClient &containerClient,
                const string &sGlob) {
  return FindBlobs(
      containerClient,
      [sGlob](const BlobItem &item) {
        return !item.IsDeleted &&
               util::glob::GitignoreGlobMatch(item.Name, sGlob);
      },
      PrefixFromGlob(sGlob));
}

static vector<BlobClient>
FindBlobs(const BlobContainerClient &containerClient,
          const function<bool(const BlobItem &item)> &Predicate,
          const string &sPrefix) {
  ListBlobsOptions listBlobsOptions;
  listBlobsOptions.Prefix = sPrefix;

  vector<BlobClient> result;
  for (auto pagedBlobList = containerClient.ListBlobs(listBlobsOptions);
       pagedBlobList.HasPage(); pagedBlobList.MoveToNextPage()) {
    for (const auto &blobItem : pagedBlobList.Blobs) {
      if (Predicate(blobItem)) {
        result.push_back(containerClient.GetBlobClient(blobItem.Name));
      }
    }
  }
  return result;
}

static string PrefixFromName(const string &sName) { return sName; }

static string PrefixFromGlob(const string &sGlob) {
  return sGlob.substr(0, util::glob::FindGlobbingChar(sGlob));
}
} // namespace az
