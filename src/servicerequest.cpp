#include "servicerequest.hpp"
#include <spdlog/spdlog.h>

using namespace std;

namespace az {
ServiceRequest::ServiceRequest(
    const Azure::Core::Url azureUrl, bool bEmulated, StorageType storageType,
    bool bDir, const BlobInfo &blob,
    shared_ptr<Azure::Storage::StorageSharedKeyCredential>
        connectionStringCredential)
    : azureUrl(azureUrl), bEmulated(bEmulated), storageType(storageType),
      bUsingConnectionString(true), bDir(bDir),
      blob{blob.sAccountName, blob.sContainer, blob.sBlob},
      connectionStringCredential(std::move(connectionStringCredential)) {
  Info();
}

ServiceRequest::ServiceRequest(
    const Azure::Core::Url azureUrl, bool bEmulated, StorageType storageType,
    bool bDir, const BlobInfo &blob,
    shared_ptr<Azure::Core::Credentials::TokenCredential>
        noConnectionStringCredential)
    : azureUrl(azureUrl), bEmulated(bEmulated), storageType(storageType),
      bUsingConnectionString(false), bDir(bDir),
      blob{blob.sAccountName, blob.sContainer, blob.sBlob},
      noConnectionStringCredential(std::move(noConnectionStringCredential)) {
  Info();
}

ServiceRequest::ServiceRequest(
    const Azure::Core::Url azureUrl, bool bEmulated, StorageType storageType,
    bool bDir, const ShareInfo &share,
    shared_ptr<Azure::Storage::StorageSharedKeyCredential>
        connectionStringCredential)
    : azureUrl(azureUrl), bEmulated(bEmulated), storageType(storageType),
      bUsingConnectionString(true), bDir(bDir), share{share.sShare, share.path},
      connectionStringCredential(std::move(connectionStringCredential)) {
  Info();
}

ServiceRequest::ServiceRequest(
    const Azure::Core::Url azureUrl, bool bEmulated, StorageType storageType,
    bool bDir, const ShareInfo &share,
    shared_ptr<Azure::Core::Credentials::TokenCredential>
        noConnectionStringCredential)
    : azureUrl(azureUrl), bEmulated(bEmulated), storageType(storageType),
      bUsingConnectionString(false), bDir(bDir),
      share{share.sShare, share.path},
      noConnectionStringCredential(std::move(noConnectionStringCredential)) {
  Info();
}

ServiceRequest::ServiceRequest(const ServiceRequest &other)
    : azureUrl(other.azureUrl), bEmulated(other.bEmulated),
      storageType(other.storageType),
      bUsingConnectionString(other.bUsingConnectionString), bDir(other.bDir),
      blob(other.blob), share(other.share),
      connectionStringCredential(other.connectionStringCredential),
      noConnectionStringCredential(other.noConnectionStringCredential) {
  Info();
}

ServiceRequest::ServiceRequest() {}

ServiceRequest &ServiceRequest::operator=(ServiceRequest &&other) {
  azureUrl = std::move(other.azureUrl);
  bEmulated = std::move(other.bEmulated);
  storageType = std::move(other.storageType);
  bUsingConnectionString = std::move(other.bUsingConnectionString);
  bDir = std::move(other.bDir);
  blob = std::move(other.blob);
  share = std::move(other.share);
  connectionStringCredential = std::move(other.connectionStringCredential);
  noConnectionStringCredential = std::move(other.noConnectionStringCredential);
  return *this;
}

void ServiceRequest::Info() {
  spdlog::debug("Created service request:");
  spdlog::debug("  URL: {}", azureUrl.GetAbsoluteUrl());
  spdlog::debug("  emulated: {}", bEmulated ? "yes" : "no");
  switch (storageType) {
  case BLOB:
    spdlog::debug("  storage type: blob");
    break;
  case SHARE:
    spdlog::debug("  storage type: file");
    break;
  default:
    spdlog::debug("  storage type: <invalid: {}>",
                  static_cast<int>(storageType));
    break;
  }
  spdlog::debug("  using connection string: {}",
                bUsingConnectionString ? "yes" : "no");
  spdlog::debug("  directory: {}", bDir ? "yes" : "no");
  if (storageType == BLOB) {
    spdlog::debug("  blob info:");
    spdlog::debug("    account name: {}", blob.sAccountName);
    spdlog::debug("    container: {}", blob.sContainer);
    spdlog::debug("    blob: {}", blob.sBlob);
  } else {
    spdlog::debug("  file info:");
    spdlog::debug("    share: {}", share.sShare);
    spdlog::debug("    path:");
    for (size_t i = 0ULL; i < share.path.size(); i++) {
      spdlog::debug("      element #{}: {}", i + 1, share.path.at(i));
    }
  }
}
}
