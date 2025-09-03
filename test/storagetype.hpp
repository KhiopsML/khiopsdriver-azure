#pragma once

enum class StorageType;

#include <ostream>

enum class StorageType
{
    BLOB,
    SHARE
};

void PrintTo(const StorageType& storageType, std::ostream* os);
