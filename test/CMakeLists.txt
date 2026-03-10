# Fetch googletest from its Git repo
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY "https://github.com/google/googletest.git"
  GIT_TAG "v1.14.0")

set(gtest_force_shared_crt
    ON
    CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

# Find dependencies
find_package(
  Boost
  COMPONENTS process
  REQUIRED)

add_executable(
  basic_test
  test_basic.cpp
  test_io.cpp
  test_end_to_end.cpp
  fixture_storage.hpp
  returnval.hpp
  utils.hpp
  urls.hpp)

target_compile_options(
  basic_test
  PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/W4;/wd4101;/wd4625;/wd4710;/wd4711;/permissive->
  PRIVATE $<$<CXX_COMPILER_ID:AppleClang>:-Wall;-Wextra;-pedantic;-Wno-unused-lambda-capture>
  PRIVATE $<$<CXX_COMPILER_ID:Clang,GNU>:-Wall;-Wextra;-pedantic>)

if(NOT DEFINED ENV{STORAGE_DRIVER_NAME})
  message(FATAL_ERROR "Environment variable STORAGE_DRIVER_NAME is not defined.")
endif()
if($ENV{STORAGE_DRIVER_NAME} STREQUAL "S3")
  target_compile_definitions(basic_test PRIVATE S3_DRIVER)
elseif($ENV{STORAGE_DRIVER_NAME} STREQUAL "GCS")
  target_compile_definitions(basic_test PRIVATE GCS_DRIVER)
elseif($ENV{STORAGE_DRIVER_NAME} STREQUAL "AZURE")
  target_compile_definitions(basic_test PRIVATE AZURE_DRIVER)
else()
  message(FATAL_ERROR "Environment variable STORAGE_DRIVER_NAME is set to '$ENV{STORAGE_DRIVER_NAME}' but should be set to 'S3', 'GCS' or 'AZURE').")
endif()

# MSVC requires /bigobj to build in debug config
target_compile_options(basic_test PRIVATE $<$<AND:$<CONFIG:Debug>,$<CXX_COMPILER_ID:MSVC>>:/bigobj>)

if(WIN32)
  target_link_libraries(
    basic_test
    PRIVATE GTest::gtest
            GTest::gmock
            GTest::gmock_main
            ${Boost_LIBRARIES}
            Azure::azure-identity
            Azure::azure-storage-blobs
            fmt::fmt
            khiopsdriver_file_azure
            bcrypt)
else()
  target_link_libraries(
    basic_test
    PRIVATE GTest::gtest
            GTest::gmock
            GTest::gmock_main
            ${Boost_LIBRARIES}
            Azure::azure-identity
            Azure::azure-storage-blobs
            fmt::fmt
            khiopsdriver_file_azure)
endif()

gtest_discover_tests(basic_test PROPERTIES ENVIRONMENT
                                           "LSAN_OPTIONS=suppressions=${CMAKE_SOURCE_DIR}/lsan_suppressions.txt")



################################################### PLUGIN TEST ###################################################
add_executable(plugin_test plugin_test.cpp path_helper.cpp)
target_include_directories(plugin_test PRIVATE ${${PROJECT_NAME}_SOURCE_DIR}/src)
target_compile_options(
  plugin_test
  PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/W4;/wd4101;/wd4710;/wd4711;/permissive->
  PRIVATE $<$<CXX_COMPILER_ID:AppleClang,Clang,GNU>:-Wall;-Wextra;-pedantic>)
if(WIN32)
  # custom build and test execution for Windows
  target_link_libraries(plugin_test PRIVATE gtest ${CMAKE_DL_LIBS} Shlwapi) # Link to dl
  add_custom_command(
    TARGET plugin_test
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_RUNTIME_DLLS:khiopsdriver_file_azure>
            $<TARGET_FILE_DIR:plugin_test>
    COMMAND_EXPAND_LISTS)
else()
  target_link_libraries(plugin_test PRIVATE gtest ${CMAKE_DL_LIBS}) # Link to dl
endif()

gtest_discover_tests(plugin_test WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory("internal")
