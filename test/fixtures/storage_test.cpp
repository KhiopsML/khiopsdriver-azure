#include "storage_test.hpp"
#include <cstdlib>
#include <string>
#include <boost/algorithm/string/case_conv.hpp>

using namespace std;

static bool IsEmulatedStorage();

void StorageTest::SetUpTestSuite()
{
    string sPrefix = IsEmulatedStorage()
        ? "http://localhost:10000/devstoreaccount1"
        : "https://khiopsdriverazure.blob.core.windows.net";

    sInexistantDirUrlAsString = sPrefix + "/data-test-khiops-driver-azure/khiops_data/bq_export/non_existent_dir/";
    sDirUrlAsString = sPrefix + "/data-test-khiops-driver-azure/khiops_data/bq_export/Adult/";
    sCreatedDirUrlAsString = sPrefix + "/data-test-khiops-driver-azure/CREATED_BY_TESTS/";
    sInexistantFileUrlAsString = sPrefix + "/data-test-khiops-driver-azure/khiops_data/samples/non_existent_file.txt";
    sFileUrlAsString = sPrefix + "/data-test-khiops-driver-azure/khiops_data/samples/Adult/Adult.txt";
    sStarGlobFileUrlAsString = sPrefix + "/data-test-khiops-driver-azure/khiops_data/bq_export/Adult/Adult-split-00000000000*.txt";

    sInexistantDirUrl = sInexistantDirUrlAsString.c_str();
    sDirUrl = sDirUrlAsString.c_str();
    sCreatedDirUrl = sCreatedDirUrlAsString.c_str();
    sInexistantFileUrl = sInexistantFileUrlAsString.c_str();
    sFileUrl = sFileUrlAsString.c_str();
    sStarGlobFileUrl = sStarGlobFileUrlAsString.c_str();
}

const char* StorageTest::sInexistantDirUrl = nullptr;
const char* StorageTest::sDirUrl = nullptr;
const char* StorageTest::sCreatedDirUrl = nullptr;
const char* StorageTest::sInexistantFileUrl = nullptr;
const char* StorageTest::sFileUrl = nullptr;
const char* StorageTest::sStarGlobFileUrl = nullptr;

string StorageTest::sInexistantDirUrlAsString = string();
string StorageTest::sDirUrlAsString = string();
string StorageTest::sCreatedDirUrlAsString = string();
string StorageTest::sInexistantFileUrlAsString = string();
string StorageTest::sFileUrlAsString = string();
string StorageTest::sStarGlobFileUrlAsString = string();

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
