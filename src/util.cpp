#define _CRT_SECURE_NO_WARNINGS // getenv would be more secure in C++ than in C
                                // and thus getenv_s would not be available in C++?
#include "util.hpp"
#include <chrono>
#include <cstdarg>
#include <cstdlib>
#include <memory>
#include <random>
#include <regex>
#include <spdlog/spdlog.h>
#include <sstream>
#include <unordered_map>

using namespace std;

namespace az {
namespace util {
namespace str {
vector<string> Split(const string &str, char delim, long long int nMaxSplits,
                     bool bRemoveEmpty) {
  size_t nStrLen = str.length();
  vector<string> fragments;
  size_t nOffset = 0;
  size_t nDelimPos;
  string sFragment;
  for (size_t nSplits = 0;
       nMaxSplits == -1 || nSplits <= static_cast<size_t>(nMaxSplits);
       nSplits++) {
    nDelimPos = nSplits == static_cast<size_t>(nMaxSplits)
                    ? string::npos
                    : str.find(delim, nOffset);
    sFragment =
        nOffset == nStrLen ? "" : str.substr(nOffset, nDelimPos - nOffset);
    if (!sFragment.empty() || !bRemoveEmpty) {
      fragments.push_back(std::move(sFragment));
    }
    if (nDelimPos == string::npos) {
      break;
    }
    nOffset = nDelimPos + 1;
  }
  return fragments;
}

bool StartsWith(const string &str, const string &prefix) {
  size_t strLen = str.length();
  size_t prefixLen = prefix.length();
  return prefixLen <= strLen && !str.compare(0, prefixLen, prefix);
}

bool EndsWith(const string &str, const string &suffix) {
  size_t strLen = str.length();
  size_t suffixLen = suffix.length();
  return suffixLen <= strLen &&
         !str.compare(strLen - suffixLen, suffixLen, suffix);
}

string ToLower(const string &str) {
  string lower(str.length(), '\0');
  transform(str.begin(), str.end(), lower.begin(),
            [](char ch) { return (char)tolower((int)ch); });
  return lower;
}
} // namespace str

namespace random {
bool RandomBool() {
  static random_device randomDevice;
  static minstd_rand::result_type seed =
      randomDevice() ^
      ((minstd_rand::result_type)chrono::duration_cast<chrono::seconds>(
           chrono::system_clock::now().time_since_epoch())
           .count() +
       (minstd_rand::result_type)chrono::duration_cast<chrono::microseconds>(
           chrono::high_resolution_clock::now().time_since_epoch())
           .count());
  static minstd_rand generator(seed);
  return (bool)(generator() % 2 == 1);
}
} // namespace random

namespace env {
string GetEnvironmentVariable(const string &sVarName) {
  char *sValue = getenv(sVarName.c_str());
  if (!sValue) {
    spdlog::debug("Environment variable {} is not set.", sVarName);
    return "";
  }
  if (strlen(sValue) == 0ULL) {
    spdlog::debug("Environment variable {} is empty.", sVarName);
    return "";
  }
  spdlog::debug("Environment variable {} is set to: {}.", sVarName, sValue);
  return sValue;
}

string GetEnvironmentVariableOrDefault(const string &sVarName,
                                       const string &sDefaultValue) {
  char *sValue = getenv(sVarName.c_str());

  if (sValue && strlen(sValue) > 0ULL) {
    return sValue;
  }

  string low_key = str::ToLower(sVarName);
  if (low_key.find("token") != string::npos ||
      low_key.find("password") != string::npos ||
      low_key.find("key") != string::npos ||
      low_key.find("secret") != string::npos) {
    spdlog::debug("No {} specified, using **REDACTED** as default.", sVarName);
  } else {
    spdlog::debug("No {} specified, using '{}' as default.", sVarName,
                  sDefaultValue);
  }

  return sDefaultValue;
}
} // namespace env

namespace connstr {
ConnectionString::ConnectionString()
    : sAccountName(""), sAccountKey(""), blobEndpointPtr(nullptr),
      fileEndpointPtr(nullptr) {}

ConnectionString::ConnectionString(const string &sAccountName,
                                   const string &sAccountKey)
    : sAccountName(sAccountName), sAccountKey(sAccountKey),
      blobEndpointPtr(nullptr), fileEndpointPtr(nullptr) {}

ConnectionString::ConnectionString(ConnectionString &&other)
    : sAccountName(move(other.sAccountName)),
      sAccountKey(move(other.sAccountKey)),
      blobEndpointPtr(move(other.blobEndpointPtr)),
      fileEndpointPtr(move(other.fileEndpointPtr)) {}

ConnectionString &ConnectionString::operator=(ConnectionString &&other) {
  sAccountName = move(other.sAccountName);
  sAccountKey = move(other.sAccountKey);
  blobEndpointPtr = move(other.blobEndpointPtr);
  fileEndpointPtr = move(other.fileEndpointPtr);
  return *this;
}

int ConnectionString::ParseConnectionString(ConnectionString *result,
                                            const string &sConnectionString,
                                            bool bIsEmulatedStorage) {
  smatch match;
  if (!regex_match(sConnectionString, match,
                   regex("(?:[^=]+=[^;]+;)*[^=]+=[^;]+;?"))) {
    spdlog::error("Connection string '{}' does not match expected pattern.", sConnectionString);
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
    spdlog::error("Connection string '{}' misses 'AccountName' field.", sConnectionString);
    return -1;
  }

  auto accountKeyIt = kvPairs.find("AccountKey"); // Mandatory
  if (accountKeyIt == kvPairs.end()) {
    spdlog::error("Connection string '{}' misses 'AccountKey' field.", sConnectionString);
    return -1;
  }

  ConnectionString connectionString(accountNameIt->second,
                                    accountKeyIt->second);

  auto blobEndpointIt =
      kvPairs.find("BlobEndpoint"); // Optional for read Azure cloud storage,
                                    // mandatory for emulated storage
  if (blobEndpointIt != kvPairs.end()) {
    connectionString.blobEndpointPtr = make_unique<Azure::Core::Url>(blobEndpointIt->second);
  } else if (bIsEmulatedStorage) {
    spdlog::error("Connection string '{}' misses 'BlobEndpoint' field.", sConnectionString);
    return -1;
  }

  auto fileEndpointIt = kvPairs.find("FileEndpoint"); // Optional
  if (fileEndpointIt != kvPairs.end()) {
    connectionString.fileEndpointPtr = make_unique<Azure::Core::Url>(fileEndpointIt->second);
  }

  spdlog::debug("Parsed connection string:");
  spdlog::debug("  account name: {}", connectionString.sAccountName);
  spdlog::debug("  account key: ***REDACTED***");
  spdlog::debug("  blob endpoint: {}", connectionString.blobEndpointPtr ? connectionString.blobEndpointPtr->GetAbsoluteUrl() : "<none>");
  spdlog::debug("  file endpoint: {}", connectionString.fileEndpointPtr ? connectionString.fileEndpointPtr->GetAbsoluteUrl() : "<none>");
  *result = move(connectionString);
  return 0;
}

int ConnectionString::CheckAgainstUrl(const Azure::Core::Url &url,
                                      StorageType storageType) const {
  // For real Azure cloud storage access, endpoints are optional, but if
  // present, check them
  if (blobEndpointPtr && storageType == BLOB &&
      !util::str::StartsWith(url.GetAbsoluteUrl(),
                             blobEndpointPtr->GetAbsoluteUrl())) {
    spdlog::error("URL {} does not start with expected blob endpoint {}.", url.GetAbsoluteUrl(), blobEndpointPtr->GetAbsoluteUrl());
    return -1;
  }
  if (fileEndpointPtr && storageType == SHARE &&
      !util::str::StartsWith(url.GetAbsoluteUrl(),
                             fileEndpointPtr->GetAbsoluteUrl())) {
    spdlog::error("URL {} does not start with expected file endpoint {}.", url.GetAbsoluteUrl(), fileEndpointPtr->GetAbsoluteUrl());
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
} // namespace connstr

namespace glob {
size_t FindGlobbingChar(const string &str) {
  smatch match;
  return regex_search(str, match,
                      regex("[^\\]([*?![^])", regex_constants::extended))
             ? match.position(1)
             : string::npos;
}
} // namespace glob
} // namespace util
} // namespace az
