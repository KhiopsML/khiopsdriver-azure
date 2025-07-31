#pragma once

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
    static std::string sStarGlobFileUrl;
};
