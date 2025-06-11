#include "azureplugin.h"
#include "azureplugin_internal.h"

#include <array>
#include <cstring>
#include <functional>
#include <iostream>
#include <limits>
#include <memory>
#include <iostream>
#include <fstream>  
#include <sstream>  

#include <boost/process/environment.hpp>

#include <boost/uuid/uuid.hpp>            // uuid class
#include <boost/uuid/uuid_generators.hpp> // generators
#include <boost/uuid/uuid_io.hpp>         // streaming operators etc.

#include <gtest/gtest.h>

using namespace azureplugin;

//using ::testing::Return;

TEST(AzureDriverTest, GetDriverName)
{
    ASSERT_STREQ(driver_getDriverName(), "Azure driver");
}

TEST(AzureDriverTest, GetVersion)
{
    ASSERT_STREQ(driver_getVersion(), DRIVER_VERSION);
}

TEST(AzureDriverTest, GetScheme)
{
    ASSERT_STREQ(driver_getScheme(), "https");
}

TEST(AzureDriverTest, IsReadOnly)
{
    ASSERT_EQ(driver_isReadOnly(), kFalse);
}

TEST(AzureDriverTest, Connect)
{
    //check connection state before call to connect
    ASSERT_EQ(driver_isConnected(), kFalse);

    //call connect and check connection
    ASSERT_EQ(driver_connect(), kSuccess);
    ASSERT_EQ(driver_isConnected(), kTrue);

    //call disconnect and check connection
    ASSERT_EQ(driver_disconnect(), kSuccess);
    ASSERT_EQ(driver_isConnected(), kFalse);
}

TEST(AzureDriverTest, Disconnect)
{
    ASSERT_EQ(driver_connect(), kSuccess);
    ASSERT_EQ(driver_disconnect(), kSuccess);
    ASSERT_EQ(driver_isConnected(), kFalse);
}

TEST(AzureDriverTest, GetFileSize)
{
	ASSERT_EQ(driver_connect(), kSuccess);
	ASSERT_EQ(driver_getFileSize(test_single_file), 5585568);
	ASSERT_EQ(driver_disconnect(), kSuccess);
}

TEST(AzureDriverTest, GetMultipartFileSize)
{
	ASSERT_EQ(driver_connect(), kSuccess);
    /// TODO: Replace URL below with actual Azure cloud storage URL
	ASSERT_EQ(driver_getFileSize(test_glob_file), 5585568);
	ASSERT_EQ(driver_disconnect(), kSuccess);
}

TEST(AzureDriverTest, GetFileSizeNonexistentFailure)
{
	ASSERT_EQ(driver_connect(), kSuccess);
    /// TODO: Replace URL below with actual Azure cloud storage URL
    ASSERT_EQ(driver_getFileSize(test_non_existent_file), -1);
    ASSERT_STRNE(driver_getlasterror(), NULL);
	ASSERT_EQ(driver_disconnect(), kSuccess);
}

TEST(AzureDriverTest, FileExists)
{
	ASSERT_EQ(driver_connect(), kSuccess);
    /// TODO: Replace URL below with actual Azure cloud storage URL
	ASSERT_EQ(driver_exist(test_single_file), kSuccess);
	ASSERT_EQ(driver_disconnect(), kSuccess);
}

TEST(AzureDriverTest, DirExists)
{
	ASSERT_EQ(driver_connect(), kSuccess);
    /// TODO: Replace URL below with actual Azure cloud storage URL
	ASSERT_EQ(driver_exist(test_dir_name), kSuccess);
	ASSERT_EQ(driver_disconnect(), kSuccess);
}

#ifndef _WIN32
// Setting of environment variables does not work on Windows
TEST(AzureDriverTest, DriverConnectMissingCredentialsFailure)
{
    FAIL(); // To be implemented
}

void setup_bad_credentials() {
    std::stringstream tempCredsFile;
#ifdef _WIN32
	tempCredsFile << std::getenv("TEMP") << "\\creds-" << boost::uuids::random_generator()() << ".json";
#else
	tempCredsFile << "/tmp/creds-" << boost::uuids::random_generator()() << ".json";
#endif
    std::ofstream outfile (tempCredsFile.str());
    outfile << "{}" << std::endl;
    outfile.close();
    auto env = boost::this_process::environment();
    env["GCP_TOKEN"] = tempCredsFile.str();
}

void cleanup_bad_credentials() {
    auto env = boost::this_process::environment();
    env.erase("GCP_TOKEN");
}

TEST(AzureDriverTest, GetFileSizeInvalidCredentialsFailure)
{
    setup_bad_credentials();
	ASSERT_EQ(driver_connect(), kSuccess);
	ASSERT_EQ(driver_getFileSize(test_single_file), -1);
    ASSERT_STRNE(driver_getlasterror(), NULL);
	ASSERT_EQ(driver_disconnect(), kSuccess);
    cleanup_bad_credentials();
}
#endif

TEST(AzureDriverTest, RmDir)
{
    ASSERT_EQ(driver_connect(), kSuccess);
	ASSERT_EQ(driver_rmdir("dummy"), kSuccess);
	ASSERT_EQ(driver_disconnect(), kSuccess);
}

TEST(AzureDriverTest, mkDir)
{
	ASSERT_EQ(driver_connect(), kSuccess);
	ASSERT_EQ(driver_mkdir("dummy"), kSuccess);
	ASSERT_EQ(driver_disconnect(), kSuccess);
}

TEST(AzureDriverTest, GetSystemPreferredBufferSize)
{
	ASSERT_EQ(driver_getSystemPreferredBufferSize(), 4 * 1024 * 1024);
}

constexpr const char* test_dir_name = "http://127.0.0.1:10000/devstoreaccount1/data-test-khiops-driver-azure/khiops_data/bq_export/Adult/";

constexpr const char* test_non_existent_file = "http://127.0.0.1:10000/devstoreaccount1/data-test-khiops-driver-azure/khiops_data/samples/non_existent_file.txt";
// Actual Azure storage URI: https://myaccount.file.core.windows.net/myshare/myfolder/myfile.txt
constexpr const char* test_single_file = "http://127.0.0.1:10000/devstoreaccount1/data-test-khiops-driver-azure/khiops_data/samples/Adult/Adult.txt";
constexpr const char* test_glob_file = "http://127.0.0.1:10000/devstoreaccount1/data-test-khiops-driver-azure/khiops_data/bq_export/Adult/Adult-split-00000000000*.txt";
constexpr const char* test_range_file_one_header = "http://127.0.0.1:10000/devstoreaccount1/data-test-khiops-driver-azure/khiops_data/split/Adult/Adult-split-0[0-5].txt";
constexpr const char* test_glob_file_header_each = "http://127.0.0.1:10000/devstoreaccount1/data-test-khiops-driver-azure/khiops_data/bq_export/Adult/*.txt";
constexpr const char* test_double_glob_header_each = "http://127.0.0.1:10000/devstoreaccount1/data-test-khiops-driver-azure/khiops_data/split/Adult_subsplit/**/Adult-split-*.txt";

constexpr std::array<const char*, 4> test_files = {
    test_single_file,
    test_range_file_one_header,
    test_glob_file_header_each,
    test_double_glob_header_each
};
