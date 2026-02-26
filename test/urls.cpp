#include "urls.hpp"
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <exception>
#include <sstream>

using namespace std;
using namespace az;

StorageTestUrlProvider::StorageTestUrlProvider() {}

StorageTestUrlProvider::StorageTestUrlProvider(StorageType storageType,
                                               bool bIsEmulatedStorage) {
  switch (storageType) {
  case BLOB:
    sPrefix = bIsEmulatedStorage
                  ? "http://localhost:10000/devstoreaccount1"
                  : "https://khiopsdriverazure.blob.core.windows.net";
    break;
  case SHARE:
    if (bIsEmulatedStorage) {
      throw runtime_error(
          "FILE storage type is incompatible with emulated mode");
    }
    sPrefix = "https://khiopsdriverazure.file.core.windows.net";
    break;
  default:
    ostringstream oss;
    oss << "invalid storage type" << (int)storageType;
    throw invalid_argument(oss.str());
  }
}

const string StorageTestUrlProvider::InexistantDir() const {
  return sPrefix + "/data-test-khiops-driver-azure/khiops_data/bq_export/"
                   "non_existent_dir/";
}

const string StorageTestUrlProvider::Dir() const {
  return sPrefix +
         "/data-test-khiops-driver-azure/khiops_data/bq_export/Adult/";
}

const string StorageTestUrlProvider::NewRandomDir() const {
  ostringstream oss;
  oss << sPrefix + "/data-test-khiops-driver-azure/khiops_data/output/"
                   "CREATED_BY_TESTS_"
      << boost::uuids::random_generator()() << "/";
  return oss.str();
}

const string StorageTestUrlProvider::InexistantFile() const {
  return sPrefix + "/data-test-khiops-driver-azure/khiops_data/samples/"
                   "non_existent_file.txt";
}

const string StorageTestUrlProvider::File() const {
  return sPrefix +
         "/data-test-khiops-driver-azure/khiops_data/samples/Adult/Adult.txt";
}

const string StorageTestUrlProvider::BQFile() const {
  return sPrefix + "/data-test-khiops-driver-azure/khiops_data/bq_export/Adult/"
                   "Adult-split-00000000000*.txt";
}

const string StorageTestUrlProvider::BQSomeFilePart() const {
  return sPrefix + "/data-test-khiops-driver-azure/khiops_data/bq_export/Adult/"
                   "Adult-split-000000000001.txt";
}

const string StorageTestUrlProvider::BQShortFilePart() const {
  return sPrefix + "/data-test-khiops-driver-azure/khiops_data/bq_export/Adult/"
                   "Adult-split-000000000002.txt";
}

const string StorageTestUrlProvider::BQEmptyFile() const {
  return sPrefix + "/data-test-khiops-driver-azure/khiops_data/bq_export/"
                   "Adult_empty/Adult-split-00000000000*.txt";
}

const string StorageTestUrlProvider::SplitFile() const {
  return sPrefix + "/data-test-khiops-driver-azure/khiops_data/split/Adult/"
                   "Adult-split-0*.txt";
}

const string StorageTestUrlProvider::MultisplitFile() const {
  return sPrefix + "/data-test-khiops-driver-azure/khiops_data/split/"
                   "Adult_subsplit/**/Adult-split-0*.txt";
}

const vector<string> StorageTestUrlProvider::SplitFileParts() const {
  return {
    sPrefix + "/data-test-khiops-driver-azure/khiops_data/split/Adult/Adult-split-00.txt",
    sPrefix + "/data-test-khiops-driver-azure/khiops_data/split/Adult/Adult-split-01.txt",
    sPrefix + "/data-test-khiops-driver-azure/khiops_data/split/Adult/Adult-split-02.txt",
    sPrefix + "/data-test-khiops-driver-azure/khiops_data/split/Adult/Adult-split-03.txt",
    sPrefix + "/data-test-khiops-driver-azure/khiops_data/split/Adult/Adult-split-04.txt",
    sPrefix + "/data-test-khiops-driver-azure/khiops_data/split/Adult/Adult-split-05.txt",
  };
}

IoTestUrlProvider::IoTestUrlProvider() {}

IoTestUrlProvider::IoTestUrlProvider(StorageType storageType,
                                     bool bIsEmulatedStorage)
    : StorageTestUrlProvider(storageType, bIsEmulatedStorage) {}

const string IoTestUrlProvider::RandomOutputFile() const {
  ostringstream oss;
  oss << sPrefix
      << "/data-test-khiops-driver-azure/khiops_data/output/CREATED_BY_TESTS_"
      << boost::uuids::random_generator()() << ".txt";
  return oss.str();
}
