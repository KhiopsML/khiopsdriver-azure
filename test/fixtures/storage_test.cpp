#include "storage_test.hpp"
#include <sstream>
#include <cstdlib>
#include <exception>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include "azureplugin.hpp"
#include "returnval.hpp"

// Macro that skips the test if emulated mode is ON,
// because the emulator does not support SHARE services.
// To be used only with SHARE-specific tests
#define SKIP_IF_EMULATED_SERVICE()                                                  \
    if (IsEmulatedStorage())                                                        \
    {                                                                               \
        GTEST_SKIP() << "emulated storage server does not support SHARE services";  \
    }

// Macro that skips the test if emulated mode is ON but storage type is SHARE,
// because the emulator does not support SHARE services.
#define SKIP_IF_EMULATED_SHARE_SERVICE()                                            \
    if (IsEmulatedStorage() && GetParam() == StorageType::SHARE)                    \
    {                                                                               \
        GTEST_SKIP() << "emulated storage server does not support SHARE services";  \
    }

using namespace std;
using namespace az;

static bool IsEmulatedStorage();

EmulatableStorageUser::EmulatableStorageUser() :
    bIsEmulatedStorage(::IsEmulatedStorage())
{
}

bool EmulatableStorageUser::IsEmulatedStorage() const
{
    return bIsEmulatedStorage;
}

// Common (blob + share) storage tests
string CommonStorageTest::FormatParam(const testing::TestParamInfo<CommonStorageTest::ParamType>& testParamInfo)
{
    ostringstream oss;
    PrintTo(testParamInfo.param, &oss);
    return oss.str();
}

void CommonStorageTest::SetUp()
{
    SKIP_IF_EMULATED_SHARE_SERVICE();
    if (IsEmulatedStorage() && GetParam() == StorageType::SHARE)
    {
        GTEST_SKIP() << "emulated storage server does not support SHARE service";
    }
    url = StorageTestUrlProvider(GetParam(), IsEmulatedStorage());
}

// Blob-specific storage tests
void BlobStorageTest::SetUp()
{
    url = StorageTestUrlProvider(StorageType::BLOB, IsEmulatedStorage());
}

// Share-specific storage tests
void ShareStorageTest::SetUp()
{
    SKIP_IF_EMULATED_SERVICE();
    url = StorageTestUrlProvider(StorageType::SHARE, IsEmulatedStorage());
}

// I/O tests (common to blob and share)
string IoTest::FormatParam(const testing::TestParamInfo<IoTest::ParamType>& testParamInfo)
{
    ostringstream oss;
    PrintTo(testParamInfo.param, &oss);
    return oss.str();
}
void IoTest::SetUp()
{
    SKIP_IF_EMULATED_SHARE_SERVICE();
    url = IoTestUrlProvider(GetParam(), IsEmulatedStorage());
#ifdef _WIN32
    sLocalFilePath = (ostringstream() << std::getenv("TEMP") << "\\out-" << boost::uuids::random_generator()() << ".txt").str();
#else
    sLocalFilePath = (ostringstream() << "/tmp/out-" << boost::uuids::random_generator()() << ".txt").str();
#endif
    ASSERT_EQ(driver_connect(), nSuccess) << "driver failed to connect during test initialization";
    ASSERT_EQ(driver_isConnected(), nTrue) << "after driver connected, it is disconnected";
}
void IoTest::TearDown()
{
    driver_disconnect();
}

// End-to-end tests (common to blob and share)
string EndToEndTest::FormatParam(const testing::TestParamInfo<EndToEndTest::ParamType>& testParamInfo)
{
    ostringstream oss;
    PrintTo(testParamInfo.param, &oss);
    return oss.str();
}
void EndToEndTest::SetUp()
{
    SKIP_IF_EMULATED_SHARE_SERVICE();
    url = EndToEndTestUrlProvider(GetParam(), IsEmulatedStorage());
#ifdef _WIN32
    sLocalFilePath = (ostringstream() << std::getenv("TEMP") << "\\out-" << boost::uuids::random_generator()() << ".txt").str();
#else
    sLocalFilePath = (ostringstream() << "/tmp/out-" << boost::uuids::random_generator()() << ".txt").str();
#endif
    ASSERT_EQ(driver_connect(), nSuccess) << "driver failed to connect during test initialization";
    ASSERT_EQ(driver_isConnected(), nTrue) << "after driver connected, it is disconnected";
}
void EndToEndTest::TearDown()
{
    driver_disconnect();
}

static bool IsEmulatedStorage()
{
    char* value = getenv("AZURE_EMULATED_STORAGE");
    return value && boost::to_lower_copy(string(value)) != "false";
}
