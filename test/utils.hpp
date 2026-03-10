#pragma once

#include "plugin.hpp"
#include "returnval.hpp"
#include <gtest/gtest.h>
#include <string>
#include <cstdlib>

enum struct StorageType { BLOB, FILE };

static StorageType GetStorageType() {
  char *test_url_prefix = getenv("STORAGE_DRIVER_TEST_URL_PREFIX");
  return test_url_prefix && std::string(test_url_prefix).find("https://khiopsdriverazure.file.core.windows.net/") == 0ULL ? StorageType::FILE : StorageType::BLOB;
}

static bool IsAzuriteStorage() {
  char *azure_emulated_storage = getenv("AZURE_EMULATED_STORAGE");
  if(azure_emulated_storage) {
    std::string azure_emulated_storage_str(azure_emulated_storage);
    if(!azure_emulated_storage_str.empty() && azure_emulated_storage_str != "false") {
      return true;
    }
  }
  return false;
}

static void CopyFile(std::string source, std::string dest) {
  void *sourceptr, *destptr;
  long long int buffersize, nread;
  ASSERT_NE((buffersize = driver_getSystemPreferredBufferSize()), -1LL) << "Could not get preferred buffer size.";
  std::vector<char> buffer(buffersize);

  ASSERT_EQ(driver_fileExists(source.c_str()), nTrue) << "Source file does not exist: '" << source << "'.";
  ASSERT_EQ(driver_fileExists(dest.c_str()), nFalse) << "Destination file already exists: '" << dest << "'.";
  ASSERT_NE((sourceptr = driver_fopen(source.c_str(), 'r')), nullptr) << "Could not open source file: '" << source << "'.";
  ASSERT_NE((destptr = driver_fopen(dest.c_str(), 'w')), nullptr) << "Could not open destination file: '" << dest << "'.";
  while((nread = driver_fread(reinterpret_cast<void *>(buffer.data()), buffersize, 1, sourceptr)) != nReadFailure) {
    ASSERT_EQ(driver_fwrite(reinterpret_cast<void *>(buffer.data()), nread, 1, destptr), nread) << "Failed to write to destination file: '" << dest << "'.";
  }
  ASSERT_EQ(driver_fclose(destptr), nCloseSuccess) << "Could not close destination file: '" << dest << "'.";
  ASSERT_EQ(driver_fclose(sourceptr), nCloseSuccess) << "Could not close source file: " << source << "'.";
  ASSERT_EQ(driver_fileExists(source.c_str()), nTrue) << "Source file does not exist anymore: '" << source << "'.";
  ASSERT_EQ(driver_fileExists(dest.c_str()), nTrue) << "Destination file has not been created: '" << dest << "'.";
}
