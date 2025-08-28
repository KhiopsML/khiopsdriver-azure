#include "storage_test.hpp"
#include <sstream>
#include <cstdlib>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include "azureplugin.hpp"
#include "returnval.hpp"

using namespace std;
using namespace az;

static bool IsEmulatedStorage();

void StorageTest::SetUpTestSuite()
{
    sUrlPrefix = IsEmulatedStorage()
        ? "http://localhost:10000/devstoreaccount1"
        : "https://khiopsdriverazure.blob.core.windows.net";

    sInexistantDirUrl = sUrlPrefix + "/data-test-khiops-driver-azure/khiops_data/bq_export/non_existent_dir/";
    sDirUrl = sUrlPrefix + "/data-test-khiops-driver-azure/khiops_data/bq_export/Adult/";
    sCreatedDirUrl = sUrlPrefix + "/data-test-khiops-driver-azure/CREATED_BY_TESTS/";
    sInexistantFileUrl = sUrlPrefix + "/data-test-khiops-driver-azure/khiops_data/samples/non_existent_file.txt";
    sFileUrl = sUrlPrefix + "/data-test-khiops-driver-azure/khiops_data/samples/Adult/Adult.txt";
    sBQFileUrl = sUrlPrefix + "/data-test-khiops-driver-azure/khiops_data/bq_export/Adult/Adult-split-00000000000*.txt";
    sBQSomePartFileUrl = sUrlPrefix + "/data-test-khiops-driver-azure/khiops_data/bq_export/Adult/Adult-split-000000000001.txt";
    sBQShortPartFileUrl = sUrlPrefix + "/data-test-khiops-driver-azure/khiops_data/bq_export/Adult/Adult-split-000000000002.txt";
    sBQEmptyFileUrl = sUrlPrefix + "/data-test-khiops-driver-azure/khiops_data/bq_export/Adult_empty/Adult-split-00000000000*.txt";
    sSplitFileUrl = sUrlPrefix + "/data-test-khiops-driver-azure/khiops_data/split/Adult/Adult-split-0*.txt";
    sMultisplitFileUrl = sUrlPrefix + "/data-test-khiops-driver-azure/khiops_data/split/Adult_subsplit/**/Adult-split-0*.txt";
}

string StorageTest::sInexistantDirUrl;
string StorageTest::sDirUrl;
string StorageTest::sCreatedDirUrl;
string StorageTest::sInexistantFileUrl;
string StorageTest::sFileUrl;
string StorageTest::sBQFileUrl;
string StorageTest::sBQSomePartFileUrl;
string StorageTest::sBQShortPartFileUrl;
string StorageTest::sBQEmptyFileUrl;
string StorageTest::sSplitFileUrl;
string StorageTest::sMultisplitFileUrl;

string StorageTest::sUrlPrefix;

static bool IsEmulatedStorage()
{
    char* value = getenv("AZURE_EMULATED_STORAGE");
    return value && boost::to_lower_copy(string(value)) != "false";
}

void AdvancedStorageTest::SetUpTestSuite()
{
    StorageTest::SetUpTestSuite();
    sOutputUrl = (ostringstream() << sUrlPrefix << "/data-test-khiops-driver-azure/khiops_data/output/" << boost::uuids::random_generator()() << "/output.txt").str();
#ifdef _WIN32
    sLocalFilePath = (ostringstream() << std::getenv("TEMP") << "\\out-" << boost::uuids::random_generator()() << ".txt").str();
#else
    sLocalFilePath = (ostringstream() << "/tmp/out-" << boost::uuids::random_generator()() << ".txt").str();
#endif
}

string AdvancedStorageTest::sOutputUrl;
string AdvancedStorageTest::sLocalFilePath;

void AdvancedStorageTest::SetUp()
{
    ASSERT_EQ(driver_connect(), nSuccess) << "driver failed to connect during test initialization";
    ASSERT_EQ(driver_isConnected(), nTrue) << "after driver connected, it is disconnected";
}

void AdvancedStorageTest::TearDown()
{
    driver_disconnect();
}
