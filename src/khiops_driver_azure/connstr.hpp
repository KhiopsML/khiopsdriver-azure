// A collection of utilities.

#pragma once

#include <memory>
#include <string>
#include "khiops_driver_azure/util.hpp"

namespace khiops_driver_azure {

struct ConnectionString {
    std::string account_name;
    std::string account_key;
    std::unique_ptr<std::string> blob_endpoint;  // optional
    std::unique_ptr<std::string> file_endpoint;  // optional
};

inline bool operator==(const ConnectionString &a, const ConnectionString &b) {
    return a.account_name == b.account_name
        && a.account_key == b.account_key
        && ((!a.blob_endpoint && !b.blob_endpoint) || (a.blob_endpoint && b.blob_endpoint && *a.blob_endpoint == *b.blob_endpoint))
        && ((!a.file_endpoint && !b.file_endpoint) || (a.file_endpoint && b.file_endpoint && *a.file_endpoint == *b.file_endpoint));
}

int ParseConnectionString(ConnectionString *result, const std::string &str, bool is_emulated_storage);
int CheckConnectionStringAgainstUrl(const ConnectionString &connection_string, const std::string &url, StorageType storage_type);

} // namespace khiops_driver_azure
