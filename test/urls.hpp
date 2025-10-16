#pragma once

class StorageTestUrlProvider;
class IoTestUrlProvider;
class EndToEndTestUrlProvider;

#include "storagetype.hpp"
#include <string>

class StorageTestUrlProvider {
public:
  StorageTestUrlProvider();
  StorageTestUrlProvider(az::StorageType storageType, bool bIsEmulatedStorage);

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

class IoTestUrlProvider : public StorageTestUrlProvider {
public:
  IoTestUrlProvider();
  IoTestUrlProvider(az::StorageType storageType, bool bIsEmulatedStorage);

  const std::string RandomOutputFile() const;
};

class EndToEndTestUrlProvider : public IoTestUrlProvider {
  using IoTestUrlProvider::IoTestUrlProvider;
};
