add_executable(internal_test test_connstring.cpp fixture_connstring.hpp)
target_compile_options(
  internal_test
  PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/W4;/wd4101;/wd4710;/wd4711;/permissive->
  PRIVATE $<$<CXX_COMPILER_ID:AppleClang,Clang,GNU>:-Wall;-Wextra;-pedantic>)
if(DEFINED ENV{STORAGE_DRIVER_TEST_URL_PREFIX} AND NOT ENV{STORAGE_DRIVER_TEST_URL_PREFIX} MATCHES "^https://khiopsdriverazure\.file\.core\.windows\.net/")
  target_compile_definitions(internal_test PRIVATE FILE_STORAGE)
endif()
if(DEFINED ENV{AZURE_EMULATED_STORAGE} AND NOT ENV{AZURE_EMULATED_STORAGE} STREQUAL "false")
  target_compile_definitions(internal_test PRIVATE AZURITE)
endif()
target_link_libraries(internal_test PRIVATE khiopsdriver_file_azure_testing Azure::azure-core GTest::gtest
                                            GTest::gmock_main)
gtest_discover_tests(internal_test)
