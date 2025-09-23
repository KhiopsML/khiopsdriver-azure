#include "azureplugin.hpp"
#include "returnval.hpp"
#include "driver.hpp"
#include "fixtures/storage_test.hpp"

#include <string>
#include <cstring>
#include <functional>
#include <iostream>
#include <limits>
#include <memory>
#include <fstream>  
#include <sstream>

#include <boost/process/v2/environment.hpp>

#include <boost/uuid/uuid.hpp>            // uuid class
#include <boost/uuid/uuid_generators.hpp> // generators
#include <boost/uuid/uuid_io.hpp>         // streaming operators etc.

#include <gtest/gtest.h>

using namespace std;
using namespace az;

void TestFSeek(string sUrl, bool bCrLf = false);

INSTANTIATE_TEST_SUITE_P(BlobAndShare, IoTest, testing::Values(BLOB, SHARE), IoTest::FormatParam);

TEST_P(IoTest, FSeekSingleFile)
{
	TestFSeek(url.File());
}

TEST_P(IoTest, FSeekMultipartFile)
{
	TestFSeek(url.MultisplitFile(), true);
}

void TestFSeek(string sUrl, bool bCrLf)
{
	void* handle;
	char* buffer = new char[32];
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

TEST_P(IoTest, FReadMultipartFileWithConcurrentWrite)
{
	string sOutputFile = url.RandomOutputFile();
	void* ihandle;
	void* ohandle;
	const char* oBuffer = "abc";
	char iBuffer[2] = { 0, 0 }; // We will read bytes one by one and the terminating NULL byte will already be there for string comparisons
	ASSERT_EQ(driver_connect(), nSuccess);
	ASSERT_NE(ohandle = driver_fopen(sOutputFile.c_str(), 'w'), nullptr);
	// Write initial data to the file
	ASSERT_EQ(driver_fwrite(oBuffer, 1, strlen(oBuffer), ohandle), strlen(oBuffer));
	ASSERT_EQ(driver_fflush(ohandle), nFlushSuccess);
	// Open the file for rading. Internally this will fetch the ETag of the file
	ASSERT_NE(ihandle = driver_fopen(sOutputFile.c_str(), 'r'), nullptr);
	// This first reading operation should find an ETag identical to the one fetched previously
	ASSERT_EQ(driver_fread(iBuffer, 1, 1, ihandle), 1);
	ASSERT_STREQ(iBuffer, "a");
	// Modify the file to change its ETag
	oBuffer = "def";
	ASSERT_EQ(driver_fwrite(oBuffer, 1, strlen(oBuffer), ohandle), strlen(oBuffer));
	ASSERT_EQ(driver_fflush(ohandle), nFlushSuccess);
	// This second reading operation should fail because it should find an ETag different to the one fetched by the driver_fopen call
	ASSERT_EQ(driver_fread(iBuffer, 1, 1, ihandle), nReadFailure);
	ASSERT_EQ(driver_fclose(ohandle), nCloseSuccess);
	ASSERT_EQ(driver_fclose(ihandle), nCloseSuccess);
	ASSERT_EQ(driver_remove(sOutputFile.c_str()), nSuccess);
	ASSERT_EQ(driver_disconnect(), nSuccess);
}
