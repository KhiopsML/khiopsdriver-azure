#include "test_uri.hpp"
#include "azureplugin.hpp"
#include "azureplugin_internal.hpp"
#include "returnval.hpp"
#include "driver.hpp"

#include <cstring>
#include <functional>
#include <iostream>
#include <limits>
#include <memory>
#include <fstream>  
#include <sstream>  

#include <boost/process/environment.hpp>

#include <boost/uuid/uuid.hpp>            // uuid class
#include <boost/uuid/uuid_generators.hpp> // generators
#include <boost/uuid/uuid_io.hpp>         // streaming operators etc.

#include <gtest/gtest.h>

using namespace azureplugin;
using namespace az;

TEST(BasicTest, GetDriverName)
{
    ASSERT_STREQ(driver_getDriverName(), "Azure driver");
}

TEST(BasicTest, GetVersion)
{
    ASSERT_STREQ(driver_getVersion(), DRIVER_VERSION);
}

TEST(BasicTest, GetScheme)
{
    ASSERT_STREQ(driver_getScheme(), "https");
}

TEST(BasicTest, IsReadOnly)
{
    ASSERT_EQ(driver_isReadOnly(), nFalse);
}

TEST(BasicTest, Connect)
{
    //check connection state before call to connect
    ASSERT_EQ(driver_isConnected(), nFalse);

    //call connect and check connection
    ASSERT_EQ(driver_connect(), nSuccess);
    ASSERT_EQ(driver_isConnected(), nTrue);

    //call disconnect and check connection
    ASSERT_EQ(driver_disconnect(), nSuccess);
    ASSERT_EQ(driver_isConnected(), nFalse);
}

TEST(BasicTest, Disconnect)
{
    ASSERT_EQ(driver_connect(), nSuccess);
    ASSERT_EQ(driver_disconnect(), nSuccess);
    ASSERT_EQ(driver_isConnected(), nFalse);
}

TEST(BasicTest, GetFileSize)
{
	ASSERT_EQ(driver_connect(), nSuccess);
	ASSERT_EQ(driver_getFileSize(test_single_file), 5585568);
	ASSERT_EQ(driver_disconnect(), nSuccess);
}

TEST(BasicTest, GetMultipartFileSize)
{
	ASSERT_EQ(driver_connect(), nSuccess);
	ASSERT_EQ(driver_getFileSize(test_glob_file), 5585568);
	ASSERT_EQ(driver_disconnect(), nSuccess);
}

TEST(BasicTest, GetFileSizeNonexistentFailure)
{
	ASSERT_EQ(driver_connect(), nSuccess);
    ASSERT_EQ(driver_getFileSize(test_non_existent_file), nSizeFailure);
    ASSERT_STRNE(driver_getlasterror(), NULL);
	ASSERT_EQ(driver_disconnect(), nSuccess);
}

TEST(BasicTest, FileExists)
{
	ASSERT_EQ(driver_connect(), nSuccess);
	ASSERT_EQ(driver_fileExists(test_single_file), nTrue);
	ASSERT_EQ(driver_disconnect(), nSuccess);
}

TEST(BasicTest, FileExistsNonExistentfile)
{
    ASSERT_EQ(driver_connect(), nSuccess);
    ASSERT_EQ(driver_fileExists(test_non_existent_file), nFalse);
    ASSERT_EQ(driver_disconnect(), nSuccess);
}

TEST(BasicTest, DirExists)
{
	ASSERT_EQ(driver_connect(), nSuccess);
	ASSERT_EQ(driver_dirExists(test_dir_name), nTrue);
	ASSERT_EQ(driver_disconnect(), nSuccess);
}

TEST(BasicTest, DirExistsNonExistentDir)
{
    ASSERT_EQ(driver_connect(), nSuccess);
    ASSERT_EQ(driver_dirExists(test_non_existent_dir), nTrue); // there is no such concept as a directory when dealing with blobs
    ASSERT_EQ(driver_disconnect(), nSuccess);
}

#ifndef _WIN32
// Setting of environment variables does not work on Windows
TEST(BasicTest, DriverConnectMissingCredentialsFailure)
{
    GTEST_SKIP() << "To be implemented.";
}

void setup_bad_credentials() {
    auto env = boost::this_process::environment();
    env["AZURE_STORAGE_CONNECTION_STRING"] =
        // Default Azurite credentials with AccountKey component slightly modified (last "w" replaced by "W")
        "DefaultEndpointsProtocol=http;AccountName=devstoreaccount1;AccountKey="
        "Eby8vdM02xNOcqFlqUwJPLlmEtlCDXJ1OUzFT50uSRZ6IFsuFq2UVErCz4I6tq/K1SZFPTOtr/"
        "KBHBeksoGMGW==;BlobEndpoint=http://127.0.0.1:10000/devstoreaccount1;";
}

void cleanup_bad_credentials() {
    auto env = boost::this_process::environment();
    env.erase("AZURE_STORAGE_CONNECTION_STRING");
}

TEST(BasicTest, GetFileSizeInvalidCredentialsFailure)
{
    setup_bad_credentials();
	ASSERT_EQ(driver_connect(), kSuccess);
	ASSERT_EQ(driver_getFileSize(test_single_file), -1);
    ASSERT_STRNE(driver_getlasterror(), NULL);
	ASSERT_EQ(driver_disconnect(), kSuccess);
    cleanup_bad_credentials();
}
#endif

TEST(BasicTest, RmDir)
{
    ASSERT_EQ(driver_connect(), nSuccess);
	ASSERT_EQ(driver_rmdir("dummy"), nSuccess);
	ASSERT_EQ(driver_disconnect(), nSuccess);
}

TEST(BasicTest, MkDir)
{
	ASSERT_EQ(driver_connect(), nSuccess);
	ASSERT_EQ(driver_mkdir("dummy"), nSuccess);
	ASSERT_EQ(driver_disconnect(), nSuccess);
}

TEST(BasicTest, GetSystemPreferredBufferSize)
{
	ASSERT_EQ(driver_getSystemPreferredBufferSize(), 4 * 1024 * 1024);
}
