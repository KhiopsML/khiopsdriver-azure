#pragma once

class ConnectionStringTest;

#include <gtest/gtest.h>

class ConnectionStringTest : public testing::TestWithParam<bool> {
protected:
  void SetUp() override { bIsEmulatedStorage = GetParam(); }
  bool bIsEmulatedStorage;
};
