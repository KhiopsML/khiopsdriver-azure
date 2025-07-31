#include "storage_test.hpp"
#include <cstdlib>
#include <string>
#include <boost/algorithm/string/case_conv.hpp>

using namespace std;

static bool IsEmulatedStorage();

void StorageTest::SetUpTestSuite()
{
    sInexistantDirUrl = uriProvider.sInexistantDir.c_str();
    sDirUrl = uriProvider.sDir.c_str();
    sCreatedDirUrl = uriProvider.sCreatedDirUrl.c_str();
    sInexistantFileUrl = uriProvider.sInexistantFile.c_str();
    sFileUrl = uriProvider.sFile.c_str();
    sStarGlobFileUrl = uriProvider.sStarGlobFile.c_str();
}

const char* StorageTest::sInexistantDirUrl = nullptr;
const char* StorageTest::sDirUrl = nullptr;
const char* StorageTest::sCreatedDirUrl = nullptr;
const char* StorageTest::sInexistantFileUrl = nullptr;
const char* StorageTest::sFileUrl = nullptr;
const char* StorageTest::sStarGlobFileUrl = nullptr;

UriProvider StorageTest::uriProvider = UriProvider(IsEmulatedStorage());

static bool IsEmulatedStorage()
{
    char* value = getenv("AZURE_EMULATED_STORAGE");
    return value && boost::to_lower_copy(string(value)) != "false";
}
