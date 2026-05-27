#pragma once

#include <azure/core.hpp>
#include <azure/storage/common/storage_credential.hpp>
#include <memory>
#include <string>
#include <vector>
#include "khiops_driver_azure/util.hpp"
#include "khiops_driver_common/backend.hpp"

namespace khiops_driver_azure {

struct ServiceRequest {
    Azure::Core::Url azure_url;
    bool is_dir;
    bool is_emulated_storage;
    StorageType storage_type;
    ObjectPath object_path;
    bool is_using_connection_string;
    std::shared_ptr<Azure::Storage::StorageSharedKeyCredential> connection_string_credential;
    std::shared_ptr<Azure::Core::Credentials::TokenCredential> no_connection_string_credential;
};

inline void LogServiceRequest(const ServiceRequest &request) {
    khiops_driver_common::GetLogger()->debug("Service request details:");
    khiops_driver_common::GetLogger()->debug("  URL: {}", request.azure_url.GetAbsoluteUrl());
    khiops_driver_common::GetLogger()->debug("  object type: {}", request.is_dir ? "directory" : "file");
    khiops_driver_common::GetLogger()->debug("  is storage emulated? {}", request.is_emulated_storage ? "yes" : "no");
    khiops_driver_common::GetLogger()->debug("  storage type: {}", request.storage_type == BLOB ? "blob" : "file share");
    khiops_driver_common::GetLogger()->debug("  object path: {}", ObjectPathToString(request.object_path));
    khiops_driver_common::GetLogger()->debug("  is using connection string? {}", request.is_using_connection_string ? "yes" : "no");
}

} // namespace khiops_driver_azure
