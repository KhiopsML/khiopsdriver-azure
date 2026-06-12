#include "connstr.hpp"
#include "khiops_driver_common/util.hpp"
#include "khiops_driver_common/logging.hpp"
#include "khiops_driver_azure/globalstate.hpp"
#include <regex>
#include <unordered_map>

using namespace std;
using namespace khiops_driver_common;

namespace khiops_driver_azure {

int ParseConnectionString(ConnectionString *result, const std::string &str, bool is_emulated_storage) {
    ConnectionString connection_string;

    smatch match;
    if (!regex_match(str, match, regex("(?:[^=]+=[^;]+;)*[^=]+=[^;]+;?"))) {
        GetLogger()->error("Connection string does not match expected pattern.");
        return -1;
    }
    regex kv_regex("([^=]+)=([^;]+);?");
    sregex_iterator begin(str.begin(), str.end(), kv_regex);
    sregex_iterator end;
    unordered_map<string, string> kv_pairs;
    for (sregex_iterator it = begin; it != end; it++) {
        kv_pairs[(*it)[1]] = (*it)[2];
    }

    auto account_name_it = kv_pairs.find("AccountName");  // mandatory
    if (account_name_it == kv_pairs.end()) {
        GetLogger()->error("Connection string misses 'AccountName' field.");
        return -1;
    }
    connection_string.account_name = account_name_it->second;

    auto account_key_it = kv_pairs.find("AccountKey");  // mandatory
    if (account_key_it == kv_pairs.end()) {
        GetLogger()->error("Connection string misses 'AccountKey' field.");
        return -1;
    }
    connection_string.account_key = account_key_it->second;

    auto blob_endpoint_it = kv_pairs.find("BlobEndpoint");  // optional for real Azure cloud storage, mandatory for emulated storage
    if (blob_endpoint_it == kv_pairs.end()) {
        if (is_emulated_storage) {
            GetLogger()->error("Connection string misses 'BlobEndpoint' field.");
            return -1;
        }
    } else /* blob endpoint found */ {
        connection_string.blob_endpoint = make_unique<string>(blob_endpoint_it->second);
    }

    auto file_endpoint_it = kv_pairs.find("FileEndpoint"); // optional
    if (file_endpoint_it != kv_pairs.end()) {
        connection_string.file_endpoint = make_unique<string>(file_endpoint_it->second);
    }

    GetLogger()->debug("Parsed connection string: account name = {}, account key = **REDACTED**, blob endpoint = {}, file endpoint = {}",
        connection_string.account_name,
        connection_string.blob_endpoint ? *connection_string.blob_endpoint : "<none>",
        connection_string.file_endpoint ? *connection_string.file_endpoint : "<none>");
    
    *result = std::move(connection_string);
    return 0;
}

int CheckConnectionStringAgainstUrl(const ConnectionString &connection_string, const string &url, StorageType storage_type) {
    // For real Azure cloud storage access, endpoints are optional, but if present, check them.
    if (storage_type == BLOB) {
        if (connection_string.blob_endpoint && !StartsWith(url, *connection_string.blob_endpoint)) {
            GetLogger()->error("URL {} does not start with expected blob endpoint {}.", url, *connection_string.blob_endpoint);
            return -1;
        }
    }
    else if (storage_type == FILE_SHARE) {
        if (connection_string.file_endpoint && !StartsWith(url, *connection_string.file_endpoint)) {
            GetLogger()->error("URL {} does not start with expected file endpoint {}.", url, *connection_string.file_endpoint);
            return -1;
        }
    }
    return 0;
}

} // namespace khiops_driver_azure
