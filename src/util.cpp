#define _CRT_SECURE_NO_WARNINGS // getenv would be more secure in C++ than in C
                                // and getenv_s in not available in C++?
#include "util.hpp"
#include <chrono>
#include <cstdlib>
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
string GetEnvironmentVariableOrThrow(const string &sVarName) {
  char *sValue = getenv(sVarName.c_str());
  if (!sValue) {
    throw EnvironmentVariableNotFoundError(sVarName);
  }
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

namespace errlog {
ErrorLogger::ErrorLogger() : sLastError("") {}

const string &ErrorLogger::GetLastError() const { return sLastError; }

void ErrorLogger::LogError(const string &error) {
  spdlog::error(sLastError = error);
}

void ErrorLogger::LogException(const exception &exc) { LogError(exc.what()); }
} // namespace errlog

namespace connstr {
ConnectionString
ConnectionString::ParseConnectionString(const string &sConnectionString) {
  smatch match;
  if (!regex_match(sConnectionString, match, regex("(?:[^=]+=[^;]+;)+"))) {
    throw ParsingError("ill-formed connection string");
  }
  regex kvRegex("([^=]+)=([^;]+);");
  sregex_iterator begin(sConnectionString.begin(), sConnectionString.end(),
                        kvRegex);
  sregex_iterator end;
  unordered_map<string, string> kvPairs;
  for (sregex_iterator it = begin; it != end; it++) {
    kvPairs[(*it)[1]] = (*it)[2];
  }
  auto accountNameIt = kvPairs.find("AccountName");
  if (accountNameIt == kvPairs.end()) {
    throw ParsingError("connection string is missing AccountName");
  }
  auto accountKeyIt = kvPairs.find("AccountKey");
  if (accountKeyIt == kvPairs.end()) {
    throw ParsingError("connection string is missing AccountKey");
  }
  auto blobEndpointIt = kvPairs.find("BlobEndpoint");
  if (blobEndpointIt == kvPairs.end()) {
    throw ParsingError("connection string is missing BlobEndpoint");
  }
  return ConnectionString{accountNameIt->second, accountKeyIt->second,
                          Azure::Core::Url(blobEndpointIt->second)};
}

bool operator==(const ConnectionString &a, const ConnectionString &b) {
  return a.sAccountName == b.sAccountName && a.sAccountKey == b.sAccountKey &&
         a.blobEndpoint.GetAbsoluteUrl() == b.blobEndpoint.GetAbsoluteUrl();
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
