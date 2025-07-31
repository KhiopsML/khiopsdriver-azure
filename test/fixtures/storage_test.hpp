#pragma once

#include <string>
#include <gtest/gtest.h>

class StorageTest : public testing::Test
{
protected:
    static void SetUpTestSuite();

    static const char* sInexistantDirUrl;
    static const char* sDirUrl;
    static const char* sCreatedDirUrl;
    static const char* sInexistantFileUrl;
    static const char* sFileUrl;
    static const char* sStarGlobFileUrl;

private:
    static std::string sInexistantDirUrlAsString;
    static std::string sDirUrlAsString;
    static std::string sCreatedDirUrlAsString;
    static std::string sInexistantFileUrlAsString;
    static std::string sFileUrlAsString;
    static std::string sStarGlobFileUrlAsString;
};
