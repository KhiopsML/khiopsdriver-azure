#include "connstr.hpp"
#include "khiops_driver_common/util.hpp"
#include "khiops_driver_common/logging.hpp"
#include <regex>
#include <unordered_map>

using namespace std;
using namespace khiops_driver_common;

namespace khiops_driver_azure {

ConnectionString::ConnectionString()
    : sAccountName(""), sAccountKey(""), blobEndpointPtr(nullptr),
      fileEndpointPtr(nullptr) {}

ConnectionString::ConnectionString(const string &sAccountName,
                                   const string &sAccountKey)
    : sAccountName(sAccountName), sAccountKey(sAccountKey),
      blobEndpointPtr(nullptr), fileEndpointPtr(nullptr) {}

ConnectionString::ConnectionString(ConnectionString &&other)
    : sAccountName(std::move(other.sAccountName)),
      sAccountKey(std::move(other.sAccountKey)),
      blobEndpointPtr(std::move(other.blobEndpointPtr)),
      fileEndpointPtr(std::move(other.fileEndpointPtr)) {}

ConnectionString &ConnectionString::operator=(ConnectionString &&other) {
  sAccountName = std::move(other.sAccountName);
  sAccountKey = std::move(other.sAccountKey);
  blobEndpointPtr = std::move(other.blobEndpointPtr);
  fileEndpointPtr = std::move(other.fileEndpointPtr);
  return *this;
}

int ConnectionString::ParseConnectionString(ConnectionString *result,
                                            const string &sConnectionString,
                                            bool bIsEmulatedStorage) {
  smatch match;
  if (!regex_match(sConnectionString, match,
                   regex("(?:[^=]+=[^;]+;)*[^=]+=[^;]+;?"))) {
    GetLogger()->error(
        "Connection string does not match expected pattern.",
        sConnectionString);
    return -1;
  }
  regex kvRegex("([^=]+)=([^;]+);?");
  sregex_iterator begin(sConnectionString.begin(), sConnectionString.end(),
                        kvRegex);
  sregex_iterator end;
  unordered_map<string, string> kvPairs;
  for (sregex_iterator it = begin; it != end; it++) {
    kvPairs[(*it)[1]] = (*it)[2];
  }

  auto accountNameIt = kvPairs.find("AccountName"); // Mandatory
  if (accountNameIt == kvPairs.end()) {
    GetLogger()->error("Connection string misses 'AccountName' field.",
                       sConnectionString);
    return -1;
  }

  auto accountKeyIt = kvPairs.find("AccountKey"); // Mandatory
  if (accountKeyIt == kvPairs.end()) {
    GetLogger()->error("Connection string misses 'AccountKey' field.",
                       sConnectionString);
    return -1;
  }

  ConnectionString connectionString(accountNameIt->second,
                                    accountKeyIt->second);

  auto blobEndpointIt =
      kvPairs.find("BlobEndpoint"); // Optional for read Azure cloud storage,
                                    // mandatory for emulated storage
  if (blobEndpointIt != kvPairs.end()) {
    connectionString.blobEndpointPtr =
        make_unique<Azure::Core::Url>(blobEndpointIt->second);
  } else if (bIsEmulatedStorage) {
    GetLogger()->error("Connection string misses 'BlobEndpoint' field.",
                       sConnectionString);
    return -1;
  }

  auto fileEndpointIt = kvPairs.find("FileEndpoint"); // Optional
  if (fileEndpointIt != kvPairs.end()) {
    connectionString.fileEndpointPtr =
        make_unique<Azure::Core::Url>(fileEndpointIt->second);
  }

  GetLogger()->debug("Parsed connection string: account name = {}, account key = **REDACTED**, blob endpoint = {}, file endpoint = {}",
    connectionString.sAccountName, connectionString.blobEndpointPtr ? connectionString.blobEndpointPtr->GetAbsoluteUrl() : "<none>",
    connectionString.fileEndpointPtr ? connectionString.fileEndpointPtr->GetAbsoluteUrl() : "<none>");
  *result = std::move(connectionString);
  return 0;
}

int ConnectionString::CheckAgainstUrl(const Azure::Core::Url &url,
                                      StorageType storageType) const {
  // For real Azure cloud storage access, endpoints are optional, but if
  // present, check them
  if (blobEndpointPtr && storageType == BLOB &&
      !StartsWith(url.GetAbsoluteUrl(),
                       blobEndpointPtr->GetAbsoluteUrl())) {
    GetLogger()->error("URL {} does not start with expected blob endpoint {}.",
                       url.GetAbsoluteUrl(), blobEndpointPtr->GetAbsoluteUrl());
    return -1;
  }
  if (fileEndpointPtr && storageType == SHARE &&
      !StartsWith(url.GetAbsoluteUrl(),
                       fileEndpointPtr->GetAbsoluteUrl())) {
    GetLogger()->error("URL {} does not start with expected file endpoint {}.",
                       url.GetAbsoluteUrl(), fileEndpointPtr->GetAbsoluteUrl());
    return -1;
  }
  return 0;
}

bool operator==(const ConnectionString &a, const ConnectionString &b) {
  // Check basic properties
  if (a.sAccountName != b.sAccountName || a.sAccountKey != b.sAccountKey) {
    return false;
  }

  // Check blob enpoint
  if (a.blobEndpointPtr ||
      b.blobEndpointPtr) // If one blob endpoint is defined...
  {
    if (!a.blobEndpointPtr || !b.blobEndpointPtr) // ... both must be defined...
    {
      return false;
    }
    if (a.blobEndpointPtr->GetAbsoluteUrl() !=
        b.blobEndpointPtr->GetAbsoluteUrl()) // ... and they must match.
    {
      return false;
    }
  }

  // Check file endpoint
  if (a.fileEndpointPtr ||
      b.fileEndpointPtr) // If one file endpoint is defined...
  {
    if (!a.fileEndpointPtr || !b.fileEndpointPtr) // ... both must be defined...
    {
      return false;
    }
    if (a.fileEndpointPtr->GetAbsoluteUrl() !=
        b.fileEndpointPtr->GetAbsoluteUrl()) // ... and they must match.
    {
      return false;
    }
  }

  return true;
}

} // namespace khiops_driver_azure
