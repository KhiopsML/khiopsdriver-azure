#pragma once

#include <memory>
#include <spdlog/spdlog.h>

namespace az {
namespace logging {

std::shared_ptr<spdlog::logger> getLogger();
const std::string &getLastError();

} // namespace logging
} // namespace az
