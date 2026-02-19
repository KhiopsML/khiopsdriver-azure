// A collection of utilities.

#pragma once

#include "storagetype.hpp"
#include <azure/core/url.hpp>
#include <memory>
#include <string>
#include <vector>

namespace az {
namespace util {
namespace str {
std::vector<std::string> Split(const std::string &str, char delim,
                               long long int nMaxSplits = -1,
                               bool bRemoveEmpty = false);
bool StartsWith(const std::string &str, const std::string &prefix);
bool EndsWith(const std::string &str, const std::string &suffix);
std::string ToLower(const std::string &str);
} // namespace str

namespace random {
bool RandomBool();
}

namespace env {
std::string GetEnvVar(const std::string &sVarName, bool bForbidLogging = false);
std::string GetEnvVarOrDefault(const std::string &sVarName,
                               const std::string &sDefaultValue, bool bForbidLogging = false);
} // namespace env

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

namespace glob {
size_t FindGlobbingChar(const std::string &str);
}
} // namespace util
} // namespace az
