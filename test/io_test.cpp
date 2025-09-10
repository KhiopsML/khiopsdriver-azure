#include "azureplugin.hpp"
#include "returnval.hpp"
#include "driver.hpp"
#include "fixtures/storage_test.hpp"

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

using namespace az;

INSTANTIATE_TEST_SUITE_P(BlobAndShare, IoTest, testing::Values(StorageType::BLOB, StorageType::SHARE), IoTest::FormatParam);

TEST_P(IoTest, FSeekSingleFile)
{
	char* buffer = new char[32];
	ASSERT_EQ(driver_connect(), nSuccess);
	void* handle = driver_fopen(url.File().c_str(), 'r');
	ASSERT_NE(handle, nullptr);

	ASSERT_EQ(driver_fseek(handle, 922, 0), nSeekSuccess);
	ASSERT_EQ(driver_fread(buffer, 1, 7, handle), 7);
	buffer[7] = 0;
	ASSERT_STREQ(buffer, "Jamaica");

	ASSERT_EQ(driver_fseek(handle, -48, 1), nSeekSuccess);
	ASSERT_EQ(driver_fread(buffer, 1, 13, handle), 13);
	buffer[13] = 0;
	ASSERT_STREQ(buffer, "Other-service");

	ASSERT_EQ(driver_fseek(handle, -658, 2), nSeekSuccess);
	ASSERT_EQ(driver_fread(buffer, 1, 13, handle), 13);
	buffer[13] = 0;
	ASSERT_STREQ(buffer, "Never-married");

	ASSERT_EQ(driver_fclose(handle), nCloseSuccess);
	ASSERT_EQ(driver_disconnect(), nSuccess);
	delete[] buffer;
}
