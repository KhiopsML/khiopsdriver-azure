#pragma once

#include <gtest/gtest.h>
#include "../uri_provider.hpp"

class StorageTest : public testing::Test
{
protected:
    static void SetUpTestSuite();

    static const char* sInexistantDirUrl;
    static const char* sDirUrl;
    static const char* sInexistantFileUrl;
    static const char* sFileUrl;
    static const char* sStarGlobFileUrl;

private:
    static UriProvider uriProvider;
};
