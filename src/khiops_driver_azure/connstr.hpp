// A collection of utilities.

#pragma once

#include "storagetype.hpp"
#include <azure/core/url.hpp>
#include <memory>
#include <string>

namespace khiops_driver_azure {
namespace connstr {

struct ConnectionString {
  std::string sAccountName;
  std::string sAccountKey;
  std::unique_ptr<Azure::Core::Url> blobEndpointPtr; // Optional
  std::unique_ptr<Azure::Core::Url> fileEndpointPtr; // Optional

  ConnectionString();
  ConnectionString(const std::string &sAccountName,
                   const std::string &sAccountKey);
  ConnectionString(ConnectionString &&other);
  ConnectionString &operator=(ConnectionString &&other);
  static int ParseConnectionString(ConnectionString *result,
                                   const std::string &sConnectionString,
                                   bool bIsEmulatedStorage);
  int CheckAgainstUrl(const Azure::Core::Url &url,
                      StorageType storageType) const;
  friend bool operator==(const ConnectionString &a, const ConnectionString &b);
};

} // namespace connstr
} // namespace khiops_driver_azure
