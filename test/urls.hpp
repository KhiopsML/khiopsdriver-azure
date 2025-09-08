#pragma once

class StorageTestUrlProvider;
class EndToEndTestUrlProvider;

#include <string>
#include "storagetype.hpp"

class StorageTestUrlProvider
{
public:
    StorageTestUrlProvider();
    StorageTestUrlProvider(StorageType storageType, bool bIsEmulatedStorage);

    const std::string InexistantDir() const;
    const std::string Dir() const;
    const std::string NewRandomDir() const;
    const std::string InexistantFile() const;
    const std::string File() const;
    const std::string BQFile() const;
    const std::string BQSomeFilePart() const;
    const std::string BQShortFilePart() const;
    const std::string BQEmptyFile() const;
    const std::string SplitFile() const;
    const std::string MultisplitFile() const;

protected:
    std::string sPrefix;
};

class EndToEndTestUrlProvider : public StorageTestUrlProvider
{
public:
    EndToEndTestUrlProvider();
    EndToEndTestUrlProvider(StorageType storageType, bool bIsEmulatedStorage);

    const std::string RandomOutputFile() const;
};
