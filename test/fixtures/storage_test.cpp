#include "storage_test.hpp"
#include <sstream>
#include <cstdlib>
#include <exception>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include "azureplugin.hpp"
#include "returnval.hpp"

using namespace std;
using namespace az;

static bool IsEmulatedStorage();

string CommonStorageTest::FormatParam(const testing::TestParamInfo<CommonStorageTest::ParamType>& testParamInfo)
{
    switch (testParamInfo.param)
    {
    case StorageType::BLOB:
        return "Blob";
    case StorageType::SHARE:
        return "File";
    default:
        throw invalid_argument((ostringstream() << "invalid storage type" << (int)testParamInfo.param).str());
    }
}

void CommonStorageTest::SetUp()
{
    url = EndToEndTestUrlProvider(GetParam(), IsEmulatedStorage());
}

StorageTestUrlProvider CommonStorageTest::url;

void BlobStorageTest::SetUpTestSuite()
{
    url = StorageTestUrlProvider(StorageType::BLOB, IsEmulatedStorage());
}

StorageTestUrlProvider BlobStorageTest::url;

void ShareStorageTest::SetUpTestSuite()
{
    url = StorageTestUrlProvider(StorageType::SHARE, IsEmulatedStorage());
}

StorageTestUrlProvider ShareStorageTest::url;

EndToEndTestUrlProvider EndToEndTest::url;
string EndToEndTest::sLocalFilePath;

void EndToEndTest::SetUp()
{
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

StorageTestUrlProvider::StorageTestUrlProvider()
{
}

StorageTestUrlProvider::StorageTestUrlProvider(StorageType storageType, bool bIsEmulatedStorage)
{
    switch (storageType)
    {
    case StorageType::BLOB:
        sPrefix = bIsEmulatedStorage ? "http://localhost:10000/devstoreaccount1" : "https://khiopsdriverazure.blob.core.windows.net";
        break;
    case StorageType::SHARE:
        if (bIsEmulatedStorage)
        {
            throw runtime_error("FILE storage type is incompatible with emulated mode");
        }
        sPrefix = "https://khiopsdriverazure.file.core.windows.net";
        break;
    default:
        throw invalid_argument((ostringstream() << "invalid storage type" << (int)storageType).str());
    }
}

const std::string StorageTestUrlProvider::InexistantDir() const
{
    return sPrefix + "/data-test-khiops-driver-azure/khiops_data/bq_export/non_existent_dir/";
}

const std::string StorageTestUrlProvider::Dir() const
{
    return sPrefix + "/data-test-khiops-driver-azure/khiops_data/bq_export/Adult/";
}

const std::string StorageTestUrlProvider::CreatedDir() const
{
    return sPrefix + "/data-test-khiops-driver-azure/CREATED_BY_TESTS/";
}

const std::string StorageTestUrlProvider::InexistantFile() const
{
    return sPrefix + "/data-test-khiops-driver-azure/khiops_data/samples/non_existent_file.txt";
}

const std::string StorageTestUrlProvider::File() const
{
    return sPrefix + "/data-test-khiops-driver-azure/khiops_data/samples/Adult/Adult.txt";
}

const std::string StorageTestUrlProvider::BQFile() const
{
    return sPrefix + "/data-test-khiops-driver-azure/khiops_data/bq_export/Adult/Adult-split-00000000000*.txt";
}

const std::string StorageTestUrlProvider::BQSomeFilePart() const
{
    return sPrefix + "/data-test-khiops-driver-azure/khiops_data/bq_export/Adult/Adult-split-000000000001.txt";
}

const std::string StorageTestUrlProvider::BQShortFilePart() const
{
    return sPrefix + "/data-test-khiops-driver-azure/khiops_data/bq_export/Adult/Adult-split-000000000002.txt";
}

const std::string StorageTestUrlProvider::BQEmptyFile() const
{
    return sPrefix + "/data-test-khiops-driver-azure/khiops_data/bq_export/Adult_empty/Adult-split-00000000000*.txt";
}

const std::string StorageTestUrlProvider::SplitFile() const
{
    return sPrefix + "/data-test-khiops-driver-azure/khiops_data/split/Adult/Adult-split-0*.txt";
}

const std::string StorageTestUrlProvider::MultisplitFile() const
{
    return sPrefix + "/data-test-khiops-driver-azure/khiops_data/split/Adult_subsplit/**/Adult-split-0*.txt";
}

EndToEndTestUrlProvider::EndToEndTestUrlProvider()
{
}

EndToEndTestUrlProvider::EndToEndTestUrlProvider(StorageType storageType, bool bIsEmulatedStorage) :
    StorageTestUrlProvider(storageType, bIsEmulatedStorage)
{
}

const std::string EndToEndTestUrlProvider::RandomOutputFile() const
{
    return (ostringstream() << sPrefix << "/data-test-khiops-driver-azure/khiops_data/output/" << boost::uuids::random_generator()() << "/output.txt").str();
}

void PrintTo(const StorageType& storageType, std::ostream* os)
{
    switch (storageType)
    {
    case StorageType::BLOB:
        *os << "BLOB";
        break;
    case StorageType::SHARE:
        *os << "FILE";
        break;
    default:
        throw invalid_argument((ostringstream() << "invalid storage type" << (int)storageType).str());
    }
}
