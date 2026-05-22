#include "servicerequest.hpp"
#include "khiops_driver_common/logging.hpp"
#include <spdlog/spdlog.h>

using namespace std;
using namespace khiops_driver_common;

namespace khiops_driver_azure {
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
  GetLogger()->debug("Created service request:");
  GetLogger()->debug("  URL: {}", azureUrl.GetAbsoluteUrl());
  GetLogger()->debug("  emulated: {}", bEmulated ? "yes" : "no");
  switch (storageType) {
  case BLOB:
    GetLogger()->debug("  storage type: blob");
    break;
  case SHARE:
    GetLogger()->debug("  storage type: file");
    break;
  default:
    GetLogger()->debug("  storage type: <invalid: {}>",
                       static_cast<int>(storageType));
    break;
  }
  GetLogger()->debug("  using connection string: {}",
                     bUsingConnectionString ? "yes" : "no");
  GetLogger()->debug("  directory: {}", bDir ? "yes" : "no");
  if (storageType == BLOB) {
    GetLogger()->debug("  blob info:");
    GetLogger()->debug("    account name: {}", blob.sAccountName);
    GetLogger()->debug("    container: {}", blob.sContainer);
    GetLogger()->debug("    blob: {}", blob.sBlob);
  } else {
    GetLogger()->debug("  file info:");
    GetLogger()->debug("    share: {}", share.sShare);
    GetLogger()->debug("    path:");
    for (size_t i = 0ULL; i < share.path.size(); i++) {
      GetLogger()->debug("      element #{}: {}", i + 1, share.path.at(i));
    }
  }
}
} // namespace khiops_driver_azure
