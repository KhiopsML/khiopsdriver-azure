#include "blobpathresolve.hpp"
#include "khiops_driver_common/contrib.hpp"
#include "khiops_driver_common/util.hpp"
#include <functional>
#include <sstream>

using namespace std;
using namespace khiops_driver_common;

namespace khiops_driver_azure {

static vector<string>
FindBlobs(const Azure::Storage::Blobs::BlobContainerClient &containerClient,
          const function<bool(const Azure::Storage::Blobs::Models::BlobItem &item)> &Predicate,
          const string &sPrefix) {
  Azure::Storage::Blobs::ListBlobsOptions listBlobsOptions;
  listBlobsOptions.Prefix = sPrefix;

  vector<string> result;
  for (auto pagedBlobList = containerClient.ListBlobs(listBlobsOptions);
       pagedBlobList.HasPage(); pagedBlobList.MoveToNextPage()) {
    for (const auto &blobItem : pagedBlobList.Blobs) {
      if (Predicate(blobItem)) {
        ostringstream oss;
        oss << containerClient.GetUrl() << "/" << blobItem.Name;
        result.push_back(oss.str());
      }
    }
  }
  return result;
}

static string PrefixFromName(const string &sName) { return sName; }

static string PrefixFromGlob(const string &sGlob) {
  return sGlob.substr(0, FindGlobbingChar(sGlob));
}

static vector<string>
FindBlobsByName(const Azure::Storage::Blobs::BlobContainerClient &containerClient,
                const string &sName) {
  return FindBlobs(
      containerClient,
      [sName](const Azure::Storage::Blobs::Models::BlobItem &item) {
        return !item.IsDeleted && item.Name == sName;
      },
      PrefixFromName(sName));
}

static vector<string>
FindBlobsByGlob(const Azure::Storage::Blobs::BlobContainerClient &containerClient,
                const string &sGlob) {
  return FindBlobs(
      containerClient,
      [sGlob](const Azure::Storage::Blobs::Models::BlobItem &item) {
        return !item.IsDeleted &&
               GitignoreGlobMatch(item.Name, sGlob);
      },
      PrefixFromGlob(sGlob));
}

vector<string>
ResolveBlobsSearchString(const Azure::Storage::Blobs::BlobContainerClient &containerClient,
                         const string &sSearchString) {
  return FindGlobbingChar(sSearchString) != string::npos
             ? FindBlobsByGlob(containerClient, sSearchString)
             : FindBlobsByName(containerClient, sSearchString);
}

} // namespace khiops_driver_azure
