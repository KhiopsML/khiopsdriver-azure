#include "azureplugin.hpp"
#include "fixtures/storage_fixture.hpp"
#include "testutil.hpp"
#include "returnval.hpp"

#include <algorithm>

#include <boost/process/v2/environment.hpp>

#include <boost/uuid/uuid.hpp>            // uuid class
#include <boost/uuid/uuid_generators.hpp> // generators
#include <boost/uuid/uuid_io.hpp>         // streaming operators etc.

#include <gtest/gtest.h>

#include <iterator>
#include <sstream>
#include <string>
#include <vector>

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

TEST_P(CommonStorageTest, Concat) {
  StorageType storageType = GetParam();

  // Define URLs
  const std::vector<std::string> sources_as_strvec = url.SplitFileParts();
  const size_t nsources = sources_as_strvec.size();
  std::vector<const char *> sources;
  std::transform(sources_as_strvec.begin(), sources_as_strvec.end(), std::back_inserter(sources), [](const std::string &source){return source.c_str();});
  const std::string outputdir = url.NewRandomDir();
  const std::string output = outputdir + "driver_concat_test_output";
  const std::string reference = url.File();
  const std::string backupdir = url.NewRandomDir();
  std::vector<std::string> backupfiles;
  std::transform(sources_as_strvec.begin(), sources_as_strvec.end(), std::back_inserter(backupfiles), [backupdir](const std::string &source){
    return backupdir + source.substr(source.rfind('/') + 1);
  });

  ASSERT_EQ(driver_connect(), nSuccess) << "Failed to connect.";
  ASSERT_EQ(driver_fileExists(output.c_str()), nFalse) << "The output file exists before concatenation.";
  // Backup sources
  if(storageType == SHARE) ASSERT_EQ(driver_dirExists(backupdir.c_str()), nFalse) << "The backup directory already exists.";
  ASSERT_EQ(driver_mkdir(backupdir.c_str()), nSuccess) << "Could not create backup directory.";
  for(size_t i = 0ULL; i < nsources; i++) {
    CopyFile(sources_as_strvec[i], backupfiles[i]);
  }
  // Concat
  if(storageType == SHARE) ASSERT_EQ(driver_dirExists(outputdir.c_str()), nFalse) << "The destination directory already exists.";
  ASSERT_EQ(driver_mkdir(outputdir.c_str()), nSuccess) << "Could not create the destination directory.";
  ASSERT_EQ(driver_concat(output.c_str(), sources.data(), nsources), nSuccess) << "Concatenation failed.";
  // Check
  for(const std::string &source : sources_as_strvec) {
    ASSERT_EQ(driver_fileExists(source.c_str()), nFalse) << "Source file " << source << " was not deleted after concatenation.";
  }
  ASSERT_EQ(driver_fileExists(output.c_str()), nTrue) << "The concatenation created no output file.";
  ASSERT_EQ(driver_getFileSize(output.c_str()), driver_getFileSize(reference.c_str())) << "Incorrect output file size.";
  // Cleanup
  ASSERT_EQ(driver_remove(output.c_str()), nSuccess) << "Failed to remove output file.";
  ASSERT_EQ(driver_fileExists(output.c_str()), nFalse) << "Output file still exists after removal.";
  ASSERT_EQ(driver_rmdir(outputdir.c_str()), nSuccess) << "Could not delete destination directory.";
  if(storageType == SHARE) ASSERT_EQ(driver_dirExists(outputdir.c_str()), nFalse) << "The destination directory still exists after removal.";
  // Restore sources
  for(size_t i = 0ULL; i < nsources; i++) {
    MoveFile(backupfiles[i], sources_as_strvec[i]);
  }
  ASSERT_EQ(driver_rmdir(backupdir.c_str()), nSuccess) << "Could not delete backup directory.";
  if(storageType == SHARE) ASSERT_EQ(driver_dirExists(backupdir.c_str()), nFalse) << "The backup directory still exists after removal.";
  ASSERT_EQ(driver_disconnect(), nSuccess) << "Failed to disconnect.";
}
