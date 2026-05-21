#include "../shared/src/khiops_driver_common/driver.h"
#include <gtest/gtest.h>

TEST(DriverMetadataTest, GetDriverName) {
  ASSERT_STREQ(driver_getDriverName(), "Azure driver");
}

TEST(DriverMetadataTest, GetVersion) {
  ASSERT_STREQ(driver_getVersion(), "0.0.8");
}

TEST(DriverMetadataTest, GetScheme) {
  ASSERT_STREQ(driver_getScheme(), "https");
}