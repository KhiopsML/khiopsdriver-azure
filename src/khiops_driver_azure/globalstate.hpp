/*
    The global state of the driver.
*/

#pragma once

#include <string>
#include <memory>
#include <azure/storage/common/storage_credential.hpp>
#include <azure/core/credentials/credentials.hpp>
#include "khiops_driver_common/filestream_management.hpp"

namespace khiops_driver_azure {

struct State {
    bool is_emulated_storage;
    bool is_using_connection_string;
    std::shared_ptr<Azure::Storage::StorageSharedKeyCredential> connection_string_credential;
    std::shared_ptr<Azure::Core::Credentials::TokenCredential> no_connection_string_credential;
#if defined(__linux__)
    std::string certificate_path;
#endif
};

inline State *GetState() {
    static std::unique_ptr<State> state = nullptr;
    if (!state) {
        state = std::make_unique<State>();
    }
    return state.get();
}

}