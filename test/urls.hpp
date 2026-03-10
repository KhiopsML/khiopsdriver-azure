#pragma once

#include <string>
#include <vector>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <exception>
#include <sstream>
#include <cstdlib>

namespace {
class StorageTestUrlProvider {
protected:
  /*
  The `sPrefix` string declared below is prepended to all test URLs.
  This prefix is set by the environment variable `STORAGE_DRIVER_TEST_URL_PREFIX`.
  The following table indicates what it should be set to.
  
  | Storage service          | Prefix                                                                        |
  | ------------------------ | ----------------------------------------------------------------------------- |
  | Amazon S3                | s3://diod-data-di-jupyterhub                                                  |
  | Google cloud storage     | gs://data-test-khiops-driver-azure                                            |
  | Azurite emulated storage | http://localhost:10000/devstoreaccount1/data-test-khiops-driver-azure         |
  | Azure cloud blob storage | https://khiopsdriverazure.blob.core.windows.net/data-test-khiops-driver-azure |
  | Azure cloud file storage | https://khiopsdriverazure.file.core.windows.net/data-test-khiops-driver-azure |
  
  > Azurite is an emulator of the Azure cloud storage that can be run on your machine.
  > The indicated prefix should work with the default Azurite configuration.
  > If you are not using the default Azurite configuration, change the prefix accordingly.
  > Note that Azurite does not support file storage emulation.
  > Also it does not support some blob storage features. For example, the one used of storage-server-side blob concatenation.
  
  > For Azurite and Azure storage services, make sure your prefix ends with a `/`, otherwise tests will not function properly.
  */
  std::string sPrefix;

private:
  static std::string GetUrlPrefix() {
    char *prefix_as_cstr = getenv("STORAGE_DRIVER_TEST_URL_PREFIX");
    if(!prefix_as_cstr) throw std::runtime_error("Environment variable STORAGE_DRIVER_TEST_URL_PREFIX not found!");
    std::string prefix(prefix_as_cstr);
    if(prefix.empty()) throw std::runtime_error("Environment variable STORAGE_DRIVER_TEST_URL_PREFIX is set to an empty value!");
    return prefix;
  }

public:
  StorageTestUrlProvider(): sPrefix(GetUrlPrefix()) {}

  const std::string InexistantDir() const {
    return sPrefix + "/khiops_data/bq_export/non_existent_dir/";
  }
  const std::string Dir() const {
    return sPrefix +
          "/khiops_data/bq_export/Adult/";
  }
  const std::string NewRandomDir() const {
    std::ostringstream oss;
    oss << sPrefix + "/khiops_data/output/CREATED_BY_TESTS_"
        << boost::uuids::random_generator()() << "/";
    return oss.str();
  }
  const std::string InexistantFile() const {
    return sPrefix + "/khiops_data/samples/non_existent_file.txt";
  }
  const std::string File() const {
    return sPrefix +
          "/khiops_data/samples/Adult/Adult.txt";
  }
  const std::string BQFile() const {
    return sPrefix + "/khiops_data/bq_export/Adult/Adult-split-00000000000*.txt";
  }
  const std::string BQSomeFilePart() const {
    return sPrefix + "/khiops_data/bq_export/Adult/Adult-split-000000000001.txt";
  }
  const std::string BQShortFilePart() const {
    return sPrefix + "/khiops_data/bq_export/Adult/Adult-split-000000000002.txt";
  }
  const std::string BQEmptyFile() const {
    return sPrefix + "/khiops_data/bq_export/Adult_empty/Adult-split-00000000000*.txt";
  }
  const std::string SplitFile() const {
    return sPrefix + "/khiops_data/split/Adult/Adult-split-0*.txt";
  }
  const std::string MultisplitFile() const {
    return sPrefix + "/khiops_data/split/Adult_subsplit/**/Adult-split-0*.txt";
  }
  const std::vector<std::string> SplitFileParts() const {
    return {
      sPrefix + "/khiops_data/split/Adult/Adult-split-00.txt",
      sPrefix + "/khiops_data/split/Adult/Adult-split-01.txt",
      sPrefix + "/khiops_data/split/Adult/Adult-split-02.txt",
      sPrefix + "/khiops_data/split/Adult/Adult-split-03.txt",
      sPrefix + "/khiops_data/split/Adult/Adult-split-04.txt",
      sPrefix + "/khiops_data/split/Adult/Adult-split-05.txt",
    };
  }

  const std::string RandomOutputFile() const {
    std::ostringstream oss;
    oss << sPrefix
        << "/khiops_data/output/CREATED_BY_TESTS_"
        << boost::uuids::random_generator()() << ".txt";
    return oss.str();
  }
};
}