#pragma once

#include <ostream>
#include "../src/storagetype.hpp"

using StorageType = az::StorageType;

void PrintTo(const StorageType& storageType, std::ostream* os);
