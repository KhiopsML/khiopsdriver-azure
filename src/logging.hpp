#pragma once

#include <memory>
#include <spdlog/spdlog.h>

namespace az {
namespace logging {

const std::shared_ptr<spdlog::logger> &getLogger();

} // namespace logging
} // namespace az