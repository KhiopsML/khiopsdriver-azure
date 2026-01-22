// A collection of utilities.

#pragma once

#include "exception.hpp"
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
class EnvironmentVariableNotFoundError : public Error {
public:
  inline EnvironmentVariableNotFoundError(const std::string &sVarName)
      : Error(concatenate("environment variable '" << sVarName
                                                   << "' not found")) {}
};

std::string GetEnvironmentVariableOrThrow(const std::string &sVarName);
std::string GetEnvironmentVariableOrDefault(const std::string &sVarName,
                                            const std::string &sDefaultValue);
} // namespace env

namespace errlog {
class ErrorLogger {
public:
  ErrorLogger();

  const std::string &GetLastError() const;
  void LogError(const std::string &error);
  void LogException(const std::exception &exc);

protected:
  std::string sLastError;
};
} // namespace errlog

namespace connstr {
class ParsingError : public Error {
  using Error::Error;
};

struct ConnectionString {
  std::string sAccountName;
  std::string sAccountKey;
  std::unique_ptr<Azure::Core::Url> blobEndpointPtr; // Optional
  std::unique_ptr<Azure::Core::Url> fileEndpointPtr; // Optional

  ConnectionString();
  ConnectionString(const std::string &sAccountName,
                   const std::string &sAccountKey);
  static ConnectionString
  ParseConnectionString(const std::string &sConnectionString,
                        bool bIsEmulatedStorage);
  ConnectionString &SetBlobEndpoint(const std::string &sUrl);
  ConnectionString &SetFileEndpoint(const std::string &sUrl);
  void CheckAgainstUrl(const Azure::Core::Url &url,
                       StorageType storageType) const;

  friend bool operator==(const ConnectionString &a, const ConnectionString &b);
};
} // namespace connstr

namespace glob {
size_t FindGlobbingChar(const std::string &str);
}
} // namespace util
} // namespace az
