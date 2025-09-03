#pragma once

class CommonStorageTest;
class BlobStorageTest;
class ShareStorageTest;
class EndToEndTest;
class StorageTestUrlProvider;
class EndToEndTestUrlProvider;
enum class StorageType;

#include <string>
#include <cstddef>
#include <tuple>
#include <ostream>
#include <gtest/gtest.h>

class CommonStorageTest : public testing::TestWithParam<StorageType>
{
public:
    static std::string FormatParam(const testing::TestParamInfo<CommonStorageTest::ParamType>& storageType);

protected:
    void SetUp() override;
    static StorageTestUrlProvider url;
};

class BlobStorageTest : public testing::Test
{
public:
    static void SetUpTestSuite();

protected:
    static StorageTestUrlProvider url;
};

class ShareStorageTest : public testing::Test
{
public:
    static void SetUpTestSuite();

protected:
    static StorageTestUrlProvider url;
};

class EndToEndTest : public CommonStorageTest
{
protected:
    void SetUp() override;
    void TearDown() override;

    static EndToEndTestUrlProvider url;
    static std::string sLocalFilePath;
};

class StorageTestUrlProvider
{
public:
    StorageTestUrlProvider();
    StorageTestUrlProvider(StorageType storageType, bool bIsEmulatedStorage);

    const std::string InexistantDir() const;
    const std::string Dir() const;
    const std::string CreatedDir() const;
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

enum class StorageType
{
    BLOB,
    SHARE
};

void PrintTo(const StorageType& storageType, std::ostream* os);
