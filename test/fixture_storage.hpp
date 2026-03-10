#pragma once

#include "urls.hpp"
#include "returnval.hpp"
#include <cstddef>
#include <gtest/gtest.h>
#include <string>

namespace {
class StorageTest : public testing::Test {
protected:
  void SetUp() override {
    url = StorageTestUrlProvider();
    std::ostringstream oss;
#ifdef _WIN32
    oss << std::getenv("TEMP") << "\\out-" << boost::uuids::random_generator()()
        << ".txt";
#else
    oss << "/tmp/out-" << boost::uuids::random_generator()() << ".txt";
#endif
    sLocalFilePath = oss.str();
    ASSERT_EQ(driver_connect(), nSuccess) << "driver failed to connect during test initialization";
    ASSERT_EQ(driver_isConnected(), nTrue) << "after driver connected, it is disconnected";
  }
  void TearDown() override { driver_disconnect(); }
  StorageTestUrlProvider url;
  std::string sLocalFilePath;
};

class IoTest : public StorageTest {};

class EndToEndTest : public StorageTest {};
}