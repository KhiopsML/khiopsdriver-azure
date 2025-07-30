#include "uri_provider.hpp"

using namespace std;

UriProvider::UriProvider(bool bIsEmulatedStorage)
{
	string sPrefix = bIsEmulatedStorage
		? "http://localhost:10000/devstoreaccount1"
		: "https://khiopsdriverazure.blob.core.windows.net";

	sInexistantDir = sPrefix + "/data-test-khiops-driver-azure/khiops_data/bq_export/non_existent_dir/";
	sDir = sPrefix + "/data-test-khiops-driver-azure/khiops_data/bq_export/Adult/";
	sInexistantFile = sPrefix + "/data-test-khiops-driver-azure/khiops_data/samples/non_existent_file.txt";
	sFile = sPrefix + "/data-test-khiops-driver-azure/khiops_data/samples/Adult/Adult.txt";
	sStarGlobFile = sPrefix + "/data-test-khiops-driver-azure/khiops_data/bq_export/Adult/Adult-split-00000000000*.txt";
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
