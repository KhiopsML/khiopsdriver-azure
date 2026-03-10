add_executable(internal_test test_connstring.cpp fixture_connstring.hpp)
target_compile_options(
  internal_test
  PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/W4;/wd4101;/wd4710;/wd4711;/permissive->
  PRIVATE $<$<CXX_COMPILER_ID:AppleClang,Clang,GNU>:-Wall;-Wextra;-pedantic>)
target_link_libraries(internal_test PRIVATE khiopsdriver_file_azure_testing Azure::azure-core GTest::gtest
                                            GTest::gmock_main)
gtest_discover_tests(internal_test)
