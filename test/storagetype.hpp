#pragma once

#include <ostream>
#include "../src/storagetype.hpp"

void PrintTo(const az::StorageType& storageType, std::ostream* os);
