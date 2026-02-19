#pragma once

#include "storagetype.hpp"
#include <azure/core.hpp>
#include <azure/storage/common/storage_credential.hpp>
#include <memory>
#include <string>
#include <vector>

namespace az {

struct BlobInfo {
  std::string sAccountName;
  std::string sContainer;
  std::string sBlob;
};

struct ShareInfo {
  std::string sShare;
  std::vector<std::string> path;
};

struct ServiceRequest {
  Azure::Core::Url azureUrl;
  bool bEmulated;
  az::StorageType storageType;
  bool bUsingConnectionString;
  bool bDir;
  BlobInfo blob;
  ShareInfo share;
  std::shared_ptr<Azure::Storage::StorageSharedKeyCredential>
      connectionStringCredential;
  std::shared_ptr<Azure::Core::Credentials::TokenCredential>
      noConnectionStringCredential;
  ServiceRequest(const Azure::Core::Url azureUrl, bool bEmulated,
                 az::StorageType storageType, bool bDir, const BlobInfo &blob,
                 std::shared_ptr<Azure::Storage::StorageSharedKeyCredential>
                     connectionStringCredential);
  ServiceRequest(const Azure::Core::Url azureUrl, bool bEmulated,
                 az::StorageType storageType, bool bDir, const BlobInfo &blob,
                 std::shared_ptr<Azure::Core::Credentials::TokenCredential>
                     noConnectionStringCredential);
  ServiceRequest(const Azure::Core::Url azureUrl, bool bEmulated,
                 az::StorageType storageType, bool bDir, const ShareInfo &share,
                 std::shared_ptr<Azure::Storage::StorageSharedKeyCredential>
                     connectionStringCredential);
  ServiceRequest(const Azure::Core::Url azureUrl, bool bEmulated,
                 az::StorageType storageType, bool bDir, const ShareInfo &share,
                 std::shared_ptr<Azure::Core::Credentials::TokenCredential>
                     noConnectionStringCredential);
  ServiceRequest(const ServiceRequest &other);
  ServiceRequest();
  ServiceRequest &operator=(ServiceRequest &&other);
  void Info();
};

} // namespace az
