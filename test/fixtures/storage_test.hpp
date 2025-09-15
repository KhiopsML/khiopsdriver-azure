#pragma once

class EmulatableStorageUser;
class CommonStorageTest;
class BlobStorageTest;
class ShareStorageTest;
class EndToEndTest;

#include <string>
#include <cstddef>
#include <gtest/gtest.h>
#include "../storagetype.hpp"
#include "../urls.hpp"

class EmulatableStorageUser
{
protected:
    EmulatableStorageUser();
    bool IsEmulatedStorage() const;

private:
    bool bIsEmulatedStorage;
};

class CommonStorageTest : public EmulatableStorageUser, public testing::TestWithParam<az::StorageType>
{
public:
    static std::string FormatParam(const testing::TestParamInfo<CommonStorageTest::ParamType>& testParamInfo);

protected:
    void SetUp() override;
    StorageTestUrlProvider url;
};

class BlobStorageTest : public EmulatableStorageUser, public testing::Test
{
protected:
    void SetUp() override;
    StorageTestUrlProvider url;
};

class ShareStorageTest : public EmulatableStorageUser, public testing::Test
{
protected:
    void SetUp() override;
    StorageTestUrlProvider url;
};

class IoTest : public EmulatableStorageUser, public testing::TestWithParam<az::StorageType>
{
public:
    static std::string FormatParam(const testing::TestParamInfo<IoTest::ParamType>& testParamInfo);

protected:
    void SetUp() override;
    void TearDown() override;

    IoTestUrlProvider url;
    std::string sLocalFilePath;
};

class EndToEndTest : public EmulatableStorageUser, public testing::TestWithParam<az::StorageType>
{
public:
    static std::string FormatParam(const testing::TestParamInfo<EndToEndTest::ParamType>& testParamInfo);

protected:
    void SetUp() override;
    void TearDown() override;

    EndToEndTestUrlProvider url;
    std::string sLocalFilePath;
};
