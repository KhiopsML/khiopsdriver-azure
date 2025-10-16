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
#include <string>

#include <boost/process/v2/environment.hpp>

#include <boost/uuid/uuid.hpp>            // uuid class
#include <boost/uuid/uuid_generators.hpp> // generators
#include <boost/uuid/uuid_io.hpp>         // streaming operators etc.

#include <gtest/gtest.h>

using namespace std;
using namespace az;

void TestFSeek(string sUrl, bool bCrLf = false);

INSTANTIATE_TEST_SUITE_P(BlobAndShare, IoTest, testing::Values(BLOB, SHARE),
                         IoTest::FormatParam);

TEST_P(IoTest, FSeekSingleFile) { TestFSeek(url.File()); }

TEST_P(IoTest, FSeekMultipartFile) { TestFSeek(url.MultisplitFile(), true); }

void TestFSeek(string sUrl, bool bCrLf) {
  void *handle;
  char *buffer = new char[32];
  ASSERT_EQ(driver_connect(), nSuccess);
  ASSERT_NE(handle = driver_fopen(sUrl.c_str(), 'r'), nullptr);

  ASSERT_EQ(driver_fseek(handle, bCrLf ? 929 : 922, 0), nSeekSuccess);
  ASSERT_EQ(driver_fread(buffer, 1, 7, handle), 7);
  buffer[7] = 0;
  ASSERT_STREQ(buffer, "Jamaica");

  ASSERT_EQ(driver_fseek(handle, -55, 1), nSeekSuccess);
  ASSERT_EQ(driver_fread(buffer, 1, 13, handle), 13);
  buffer[13] = 0;
  ASSERT_STREQ(buffer, "Other-service");

  ASSERT_EQ(driver_fseek(handle, bCrLf ? -664 : -658, 2), nSeekSuccess);
  ASSERT_EQ(driver_fread(buffer, 1, 13, handle), 13);
  buffer[13] = 0;
  ASSERT_STREQ(buffer, "Never-married");

  ASSERT_EQ(driver_fclose(handle), nCloseSuccess);
  ASSERT_EQ(driver_disconnect(), nSuccess);
  delete[] buffer;
}

TEST_P(IoTest, FReadAtEndOfFile) {
  char ibuffer[64];
  void *ihandle;
  long long int filesize;

  ASSERT_EQ(driver_connect(), nSuccess);

  // We want the file to be at least 10-byte long
  ASSERT_GT(filesize = driver_getFileSize(url.File().c_str()), 10);

  ASSERT_NE(ihandle = driver_fopen(url.File().c_str(), 'r'), nullptr);

  // Reading the first four bytes... OK
  ASSERT_EQ(driver_fread(ibuffer, 1, 4, ihandle), 4);
  ibuffer[4] = 0;
  ASSERT_STREQ(ibuffer, "Labe");
  // Reading the next four bytes... OK
  ASSERT_EQ(driver_fread(ibuffer, 1, 4, ihandle), 4);
  ibuffer[4] = 0;
  ASSERT_STREQ(ibuffer, "l\tag");
  ASSERT_EQ(driver_fseek(ihandle, -2, 2), nSeekSuccess);
  // Trying to read four bytes from the last but one... it should read the last
  // two bytes
  ASSERT_EQ(driver_fread(ibuffer, 1, 4, ihandle), 2);
  ibuffer[2] = 0;
  ASSERT_STREQ(ibuffer, "e\n");
  // Trying to read four bytes while we are already at the end of the file...
  // should raise an error
  ASSERT_EQ(driver_fread(ibuffer, 1, 4, ihandle), nReadFailure);
  ASSERT_STREQ(driver_getlasterror(), "cannot read after end of file");
  ASSERT_STREQ(ibuffer, "e\n"); // Buffer content unchanged

  ASSERT_EQ(driver_fclose(ihandle), nCloseSuccess);

  ASSERT_EQ(driver_disconnect(), nSuccess);
}

TEST_P(IoTest, FReadWithConcurrentWrite) {
  string file = url.RandomOutputFile();
  char ibuffer[64]{};
  void *ihandle;
  void *ohandle;

  ASSERT_EQ(driver_connect(), nSuccess);

  // Write initial data to the file
  ASSERT_NE(ohandle = driver_fopen(file.c_str(), 'w'), nullptr);
  ASSERT_EQ(driver_fwrite("abc", 1, 3, ohandle), 3);
  ASSERT_EQ(driver_fflush(ohandle), nFlushSuccess);

  // Open the file for reading. Internally this will fetch the ETag of the file
  ASSERT_NE(ihandle = driver_fopen(file.c_str(), 'r'), nullptr);
  // This first reading operation should find an ETag identical to the one
  // fetched previously
  ASSERT_EQ(driver_fread(ibuffer, 1, 3, ihandle), 3);
  ASSERT_STREQ(ibuffer, "abc");

  // Add some data to the file. The ETag of the file will be changed
  ASSERT_EQ(driver_fwrite("def", 1, 3, ohandle), 3);
  ASSERT_EQ(driver_fflush(ohandle), nFlushSuccess);

  // This second reading operation should fail because it should find an ETag
  // different to the one fetched by the driver_fopen call
  ASSERT_EQ(driver_fread(ibuffer, 1, 6, ihandle), nReadFailure);
  ASSERT_STREQ(driver_getlasterror(),
               "the file has been updated during the reading");
  ASSERT_STREQ(ibuffer, "abc"); // Input buffer content unchanged

  // Open file again. This will fetch the new ETag
  ASSERT_EQ(driver_fclose(ihandle), nCloseSuccess);
  ASSERT_NE(ihandle = driver_fopen(file.c_str(), 'r'), nullptr);
  // Now read the content again. It should be the new one
  ASSERT_EQ(driver_fread(ibuffer, 1, 6, ihandle), 6);
  ASSERT_STREQ(ibuffer, "abcdef");

  ASSERT_EQ(driver_fclose(ihandle), nCloseSuccess);
  ASSERT_EQ(driver_fclose(ohandle), nCloseSuccess);
  ASSERT_EQ(driver_remove(file.c_str()), nSuccess);

  ASSERT_EQ(driver_disconnect(), nSuccess);
}
