#pragma once

#include "azureplugin.hpp"
#include "returnval.hpp"
#include <gtest/gtest.h>
#include <string>

static void CopyFile(std::string source, std::string dest) {
  void *sourceptr, *destptr;
  long long int buffersize, nread;
  ASSERT_NE((buffersize = driver_getSystemPreferredBufferSize()), -1LL) << "Could not get preferred buffer size.";
  std::vector<char> buffer(buffersize);

  ASSERT_EQ(driver_fileExists(source.c_str()), az::nTrue) << "Source file does not exist: '" << source << "'.";
  ASSERT_EQ(driver_fileExists(dest.c_str()), az::nFalse) << "Destination file already exists: '" << dest << "'.";
  ASSERT_NE((sourceptr = driver_fopen(source.c_str(), 'r')), nullptr) << "Could not open source file: '" << source << "'.";
  ASSERT_NE((destptr = driver_fopen(dest.c_str(), 'w')), nullptr) << "Could not open destination file: '" << dest << "'.";
  while((nread = driver_fread(reinterpret_cast<void *>(buffer.data()), buffersize, 1, sourceptr)) != az::nReadFailure) {
    ASSERT_EQ(driver_fwrite(reinterpret_cast<void *>(buffer.data()), nread, 1, destptr), nread) << "Failed to write to destination file: '" << dest << "'.";
  }
  ASSERT_EQ(driver_fclose(destptr), az::nCloseSuccess) << "Could not close destination file: '" << dest << "'.";
  ASSERT_EQ(driver_fclose(sourceptr), az::nCloseSuccess) << "Could not close source file: " << source << "'.";
  ASSERT_EQ(driver_fileExists(source.c_str()), az::nTrue) << "Source file does not exist anymore: '" << source << "'.";
  ASSERT_EQ(driver_fileExists(dest.c_str()), az::nTrue) << "Destination file has not been created: '" << dest << "'.";
}
