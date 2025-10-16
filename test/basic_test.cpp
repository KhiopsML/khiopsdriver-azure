#include "azureplugin.hpp"
#include "driver.hpp"
#include "fixtures/storage_test.hpp"
#include "returnval.hpp"

#include <cstring>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>

#include <boost/process/v2/environment.hpp>

#include <boost/uuid/uuid.hpp>            // uuid class
#include <boost/uuid/uuid_generators.hpp> // generators
#include <boost/uuid/uuid_io.hpp>         // streaming operators etc.

#include <gtest/gtest.h>

using namespace az;

TEST(BasicTest, GetDriverName) {
  ASSERT_STREQ(driver_getDriverName(), "Azure driver");
}

TEST(BasicTest, GetVersion) {
  ASSERT_STREQ(driver_getVersion(), DRIVER_VERSION);
}

TEST(BasicTest, GetScheme) { ASSERT_STREQ(driver_getScheme(), "https"); }

TEST(BasicTest, IsReadOnly) { ASSERT_EQ(driver_isReadOnly(), nFalse); }

TEST(BasicTest, GetSystemPreferredBufferSize) {
  ASSERT_EQ(driver_getSystemPreferredBufferSize(), 4 * 1024 * 1024);
}

TEST(BasicTest, Connect) {
  // check connection state before call to connect
  ASSERT_EQ(driver_isConnected(), nFalse);

  // call connect and check connection
  ASSERT_EQ(driver_connect(), nSuccess);
  ASSERT_EQ(driver_isConnected(), nTrue);

  // call disconnect and check connection
  ASSERT_EQ(driver_disconnect(), nSuccess);
  ASSERT_EQ(driver_isConnected(), nFalse);
}

TEST(BasicTest, Disconnect) {
  ASSERT_EQ(driver_connect(), nSuccess);
  ASSERT_EQ(driver_disconnect(), nSuccess);
  ASSERT_EQ(driver_isConnected(), nFalse);
}

INSTANTIATE_TEST_SUITE_P(BlobAndShare, CommonStorageTest,
                         testing::Values(BLOB, SHARE),
                         CommonStorageTest::FormatParam);

TEST_P(CommonStorageTest, GetFileSize) {
  ASSERT_EQ(driver_connect(), nSuccess);
  ASSERT_EQ(driver_getFileSize(url.File().c_str()), 5585568);
  ASSERT_EQ(driver_disconnect(), nSuccess);
}

TEST_P(CommonStorageTest, GetMultipartFileSize) {
  ASSERT_EQ(driver_connect(), nSuccess);
  ASSERT_EQ(driver_getFileSize(url.BQFile().c_str()), 5585568);
  ASSERT_EQ(driver_disconnect(), nSuccess);
}

TEST_P(CommonStorageTest, GetFileSizeNonexistentFailure) {
  ASSERT_EQ(driver_connect(), nSuccess);
  ASSERT_EQ(driver_getFileSize(url.InexistantFile().c_str()), nSizeFailure);
  ASSERT_STRNE(driver_getlasterror(), NULL);
  ASSERT_EQ(driver_disconnect(), nSuccess);
}

TEST_P(CommonStorageTest, FileExists) {
  ASSERT_EQ(driver_connect(), nSuccess);
  ASSERT_EQ(driver_fileExists(url.File().c_str()), nTrue);
  ASSERT_EQ(driver_disconnect(), nSuccess);
}

TEST_P(CommonStorageTest, FileExistsNonExistentfile) {
  ASSERT_EQ(driver_connect(), nSuccess);
  ASSERT_EQ(driver_fileExists(url.InexistantFile().c_str()), nFalse);
  ASSERT_EQ(driver_disconnect(), nSuccess);
}

TEST_F(BlobStorageTest, DirExists) {
  ASSERT_EQ(driver_connect(), nSuccess);
  ASSERT_EQ(
      driver_dirExists(url.Dir().c_str()),
      nTrue); // there is no such concept as a directory when dealing with blobs
  ASSERT_EQ(driver_disconnect(), nSuccess);
}

TEST_F(ShareStorageTest, DirExists) {
  ASSERT_EQ(driver_connect(), nSuccess);
  ASSERT_EQ(driver_dirExists(url.Dir().c_str()), nTrue);
  ASSERT_EQ(driver_disconnect(), nSuccess);
}

TEST_F(BlobStorageTest, DirExistsNonExistentDir) {
  ASSERT_EQ(driver_connect(), nSuccess);
  ASSERT_EQ(
      driver_dirExists(url.InexistantDir().c_str()),
      nTrue); // there is no such concept as a directory when dealing with blobs
  ASSERT_EQ(driver_disconnect(), nSuccess);
}

TEST_F(ShareStorageTest, DirExistsNonExistentDir) {
  ASSERT_EQ(driver_connect(), nSuccess);
  ASSERT_EQ(driver_dirExists(url.InexistantDir().c_str()), nFalse);
  ASSERT_EQ(driver_disconnect(), nSuccess);
}

#ifndef _WIN32
// Setting of environment variables does not work on Windows
TEST(BasicTest, DriverConnectMissingCredentialsFailure) {
  GTEST_SKIP() << "To be implemented.";
}

void setup_bad_credentials() {
  boost::process::v2::environment::set(
      "AZURE_STORAGE_CONNECTION_STRING",
      // Default Azurite credentials with AccountKey component slightly modified
      // (last "w" replaced by "W")
      "DefaultEndpointsProtocol=http;AccountName=devstoreaccount1;AccountKey="
      "Eby8vdM02xNOcqFlqUwJPLlmEtlCDXJ1OUzFT50uSRZ6IFsuFq2UVErCz4I6tq/"
      "K1SZFPTOtr/"
      "KBHBeksoGMGW==;BlobEndpoint=http://localhost:10000/devstoreaccount1;");
}

void cleanup_bad_credentials() {
  boost::process::v2::environment::unset("AZURE_STORAGE_CONNECTION_STRING");
}

TEST_P(CommonStorageTest, GetFileSizeInvalidCredentialsFailure) {
  GTEST_SKIP() << "To be fixed.";
  setup_bad_credentials();
  ASSERT_EQ(driver_connect(), nSuccess);
  ASSERT_EQ(driver_getFileSize(url.File().c_str()), -1);
  ASSERT_STRNE(driver_getlasterror(), NULL);
  ASSERT_EQ(driver_disconnect(), nSuccess);
  cleanup_bad_credentials();
}
#endif

TEST_F(BlobStorageTest, MkDir) {
  ASSERT_EQ(driver_connect(), nSuccess);
  ASSERT_EQ(driver_mkdir(url.NewRandomDir().c_str()),
            nSuccess); // there is no such concept as a directory when dealing
                       // with blobs
  ASSERT_EQ(driver_disconnect(), nSuccess);
}

TEST_F(ShareStorageTest, MkDir) {
  std::string sNewDir = url.NewRandomDir();
  ASSERT_EQ(driver_connect(), nSuccess);
  ASSERT_EQ(driver_dirExists(sNewDir.c_str()), nFalse);
  ASSERT_EQ(driver_mkdir(sNewDir.c_str()), nSuccess);
  ASSERT_EQ(driver_dirExists(sNewDir.c_str()), nTrue);
  ASSERT_EQ(driver_rmdir(sNewDir.c_str()), nSuccess);
  ASSERT_EQ(driver_disconnect(), nSuccess);
}

TEST_F(BlobStorageTest, RmDir) {
  ASSERT_EQ(driver_connect(), nSuccess);
  ASSERT_EQ(driver_rmdir(url.NewRandomDir().c_str()),
            nSuccess); // there is no such concept as a directory when dealing
                       // with blobs
  ASSERT_EQ(driver_disconnect(), nSuccess);
}

TEST_F(ShareStorageTest, RmDir) {
  std::string sNewDir = url.NewRandomDir();
  ASSERT_EQ(driver_connect(), nSuccess);
  ASSERT_EQ(driver_mkdir(sNewDir.c_str()), nSuccess);
  ASSERT_EQ(driver_dirExists(sNewDir.c_str()), nTrue);
  ASSERT_EQ(driver_rmdir(sNewDir.c_str()), nSuccess);
  ASSERT_EQ(driver_dirExists(sNewDir.c_str()), nFalse);
  ASSERT_EQ(driver_disconnect(), nSuccess);
}
