#include "../shared/src/khiops_driver_common/driver.h"
#include "../src/khiops_driver_azure/version.hpp"
#include <gtest/gtest.h>

TEST(DriverMetadataTest, GetDriverName) {
  ASSERT_STREQ(driver_getDriverName(), "Azure driver");
}

TEST(DriverMetadataTest, GetVersion) {
  ASSERT_STREQ(driver_getVersion(), DRIVER_VERSION);
}

TEST(DriverMetadataTest, GetScheme) {
  ASSERT_STREQ(driver_getScheme(), "https");
}