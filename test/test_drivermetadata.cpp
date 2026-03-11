#include <gtest/gtest.h>
#include "../src/azureplugin.hpp"

TEST(DriverMetadataTest, GetDriverName) {
  ASSERT_STREQ(driver_getDriverName(), "Azure driver");
}

TEST(DriverMetadataTest, GetVersion) {
  ASSERT_STREQ(driver_getVersion(), DRIVER_VERSION);
}

TEST(DriverMetadataTest, GetScheme) { ASSERT_STREQ(driver_getScheme(), "https"); }