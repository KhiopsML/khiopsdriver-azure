#pragma once

class StorageTest;
struct AdvancedStorageTestInput;
class AdvancedStorageTest;

#include <string>
#include <cstddef>
#include <gtest/gtest.h>

class StorageTest : public testing::Test
{
protected:
    static void SetUpTestSuite();

    static std::string sInexistantDirUrl;
    static std::string sDirUrl;
    static std::string sCreatedDirUrl;
    static std::string sInexistantFileUrl;
    static std::string sFileUrl;
    static std::string sBQFileUrl;
    static std::string sBQSomePartFileUrl;
    static std::string sBQShortPartFileUrl;
    static std::string sBQEmptyFileUrl;
    static std::string sSplitFileUrl;
    static std::string sMultisplitFileUrl;

    static std::string sUrlPrefix;
};

class AdvancedStorageTest : public StorageTest
{
protected:
    static void SetUpTestSuite();

    static std::string sOutputUrl;
    static std::string sLocalFilePath;

    void SetUp() override;
    void TearDown() override;

    std::string sInputUrl;
    size_t nBufferSize;
};
