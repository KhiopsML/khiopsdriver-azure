#pragma once

class StorageTest;
class AdvancedStorageTest;

#include <string>
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
};

class AdvancedStorageTest : public StorageTest
{
};
