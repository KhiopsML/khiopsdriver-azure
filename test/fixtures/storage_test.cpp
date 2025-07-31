#include "storage_test.hpp"
#include <cstdlib>
#include <boost/algorithm/string/case_conv.hpp>

using namespace std;

static bool IsEmulatedStorage();

void StorageTest::SetUpTestSuite()
{
    string sPrefix = IsEmulatedStorage()
        ? "http://localhost:10000/devstoreaccount1"
        : "https://khiopsdriverazure.blob.core.windows.net";

    sInexistantDirUrl = sPrefix + "/data-test-khiops-driver-azure/khiops_data/bq_export/non_existent_dir/";
    sDirUrl = sPrefix + "/data-test-khiops-driver-azure/khiops_data/bq_export/Adult/";
    sCreatedDirUrl = sPrefix + "/data-test-khiops-driver-azure/CREATED_BY_TESTS/";
    sInexistantFileUrl = sPrefix + "/data-test-khiops-driver-azure/khiops_data/samples/non_existent_file.txt";
    sFileUrl = sPrefix + "/data-test-khiops-driver-azure/khiops_data/samples/Adult/Adult.txt";
    sBQFileUrl = sPrefix + "/data-test-khiops-driver-azure/khiops_data/bq_export/Adult/Adult-split-00000000000*.txt";
    sBQSomePartFileUrl = sPrefix + "/data-test-khiops-driver-azure/khiops_data/bq_export/Adult/Adult-split-000000000001.txt";
    sBQShortPartFileUrl = sPrefix + "/data-test-khiops-driver-azure/khiops_data/bq_export/Adult/Adult-split-000000000002.txt";
    sBQEmptyFileUrl = sPrefix + "/data-test-khiops-driver-azure/khiops_data/bq_export/Adult_empty/Adult-split-00000000000*.txt";
    sSplitFileUrl = sPrefix + "/data-test-khiops-driver-azure/khiops_data/split/Adult/Adult-split-0*.txt";
    sMultisplitFileUrl = sPrefix + "/data-test-khiops-driver-azure/khiops_data/split/Adult_subsplit/**/Adult-split-0*.txt";
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

static bool IsEmulatedStorage()
{
    char* value = getenv("AZURE_EMULATED_STORAGE");
    return value && boost::to_lower_copy(string(value)) != "false";
}

//constexpr const char* test_dir_name = "http://localhost:10000/devstoreaccount1/data-test-khiops-driver-azure/khiops_data/bq_export/Adult/";
//constexpr const char* test_non_existent_dir = "http://localhost:10000/devstoreaccount1/data-test-khiops-driver-azure/khiops_data/bq_export/non_existent_dir/";
//
//constexpr const char* test_non_existent_file = "http://localhost:10000/devstoreaccount1/data-test-khiops-driver-azure/khiops_data/samples/non_existent_file.txt";
//// Actual Azure storage URI: https://myaccount.file.core.windows.net/myshare/myfolder/myfile.txt
//constexpr const char* test_single_file = "http://localhost:10000/devstoreaccount1/data-test-khiops-driver-azure/khiops_data/samples/Adult/Adult.txt";
////constexpr const char* test_single_file = "https://khiopsdriverazure.blob.core.windows.net/data-test-khiops-driver-azure/khiops_data/samples/Adult/Adult.txt";
//constexpr const char* test_glob_file = "http://localhost:10000/devstoreaccount1/data-test-khiops-driver-azure/khiops_data/bq_export/Adult/Adult-split-00000000000*.txt";
//constexpr const char* test_range_file_one_header = "http://localhost:10000/devstoreaccount1/data-test-khiops-driver-azure/khiops_data/split/Adult/Adult-split-0[0-5].txt";
//constexpr const char* test_glob_file_header_each = "http://localhost:10000/devstoreaccount1/data-test-khiops-driver-azure/khiops_data/bq_export/Adult/*.txt";
//constexpr const char* test_double_glob_header_each = "http://localhost:10000/devstoreaccount1/data-test-khiops-driver-azure/khiops_data/split/Adult_subsplit/**/Adult-split-*.txt";
