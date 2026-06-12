#include "khiops_driver_azure/connstr.hpp"
#include "fixture_connstring.hpp"
#include <azure/core/url.hpp>
#include <gtest/gtest.h>
#include <memory>

const char *sValidConnString =
"AccountName=devstoreaccount1;"
"AccountKey=Eby8vdM02xNOcqFlqUwJPLlmEtlCDXJ1OUzFT50uSRZ6IFsuFq2UVErCz4I6tq/"
"K1SZFPTOtr/KBHBeksoGMGw==;"
"DefaultEndpointsProtocol=http;"
"BlobEndpoint=http://127.0.0.1:10000/devstoreaccount1;"
"QueueEndpoint=http://127.0.0.1:10001/devstoreaccount1;"
"TableEndpoint=http://127.0.0.1:10002/devstoreaccount1;";

const char *sValidConnStringWithoutTrailingSemicolon =
    "AccountName=devstoreaccount1;"
    "AccountKey=Eby8vdM02xNOcqFlqUwJPLlmEtlCDXJ1OUzFT50uSRZ6IFsuFq2UVErCz4I6tq/"
    "K1SZFPTOtr/KBHBeksoGMGw==;"
    "DefaultEndpointsProtocol=http;"
    "BlobEndpoint=http://127.0.0.1:10000/devstoreaccount1;"
    "QueueEndpoint=http://127.0.0.1:10001/devstoreaccount1;"
    "TableEndpoint=http://127.0.0.1:10002/devstoreaccount1";

    const char *sIllFormedConnString =
    "AccountName=devstoreaccount1;"
    "AccountKey=Eby8vdM02xNOcqFlqUwJPLlmEtlCDXJ1OUzFT50uSRZ6IFsuFq2UVErCz4I6tq/"
    "K1SZFPTOtr/KBHBeksoGMGw==;"
    "DefaultEndpointsProtocol=http;"
    "BlobEndpoint=http://127.0.0.1:10000/devstoreaccount1;"
    "QueueEndpoint=http://127.0.0.1:10001/devstoreaccount1;"
    "TableEndpointhttp://127.0.0.1:10002/devstoreaccount1;"; // Missing equal
    // sign in last
    // key-value pair

const char *sConnStringMissingAccountName =
"AccountKey=Eby8vdM02xNOcqFlqUwJPLlmEtlCDXJ1OUzFT50uSRZ6IFsuFq2UVErCz4I6tq/"
"K1SZFPTOtr/KBHBeksoGMGw==;"
"DefaultEndpointsProtocol=http;"
"BlobEndpoint=http://127.0.0.1:10000/devstoreaccount1;"
"QueueEndpoint=http://127.0.0.1:10001/devstoreaccount1;"
"TableEndpoint=http://127.0.0.1:10002/devstoreaccount1;";

const char *sConnStringMissingAccountKey =
"AccountName=devstoreaccount1;"
"DefaultEndpointsProtocol=http;"
"BlobEndpoint=http://127.0.0.1:10000/devstoreaccount1;"
"QueueEndpoint=http://127.0.0.1:10001/devstoreaccount1;"
"TableEndpoint=http://127.0.0.1:10002/devstoreaccount1;";

const char *sConnStringMissingBlobEndpoint =
"AccountName=devstoreaccount1;"
"AccountKey=Eby8vdM02xNOcqFlqUwJPLlmEtlCDXJ1OUzFT50uSRZ6IFsuFq2UVErCz4I6tq/"
"K1SZFPTOtr/KBHBeksoGMGw==;"
"DefaultEndpointsProtocol=http;"
"QueueEndpoint=http://127.0.0.1:10001/devstoreaccount1;"
"TableEndpoint=http://127.0.0.1:10002/devstoreaccount1;";

using namespace khiops_driver_azure;

INSTANTIATE_TEST_SUITE_P(EmulatedAndNotEmulatedStorage, ConnectionStringTest,
    testing::Values(false, true));
    
    TEST_P(ConnectionStringTest, ParseValidConnString) {
        ConnectionString actual;
        ASSERT_EQ(ParseConnectionString(
            &actual, sValidConnString, bIsEmulatedStorage),
            0);
  ConnectionString expected;
  expected.account_name = "devstoreaccount1";
  expected.account_key = "Eby8vdM02xNOcqFlqUwJPLlmEtlCDXJ1OUzFT50uSRZ6IFsuFq2UVErCz4I6tq/K1SZFPTOtr/KBHBeksoGMGw==";
  expected.blob_endpoint = std::make_unique<std::string>("http://127.0.0.1:10000/devstoreaccount1");
  ASSERT_EQ(actual, expected);
}

TEST_P(ConnectionStringTest, ParseValidConnStringWithoutTrailingSemicolon) {
  ConnectionString actual;
  ASSERT_EQ(ParseConnectionString(
                &actual, sValidConnStringWithoutTrailingSemicolon,
                bIsEmulatedStorage),
            0);
  ConnectionString expected;
  expected.account_name = "devstoreaccount1";
  expected.account_key = "Eby8vdM02xNOcqFlqUwJPLlmEtlCDXJ1OUzFT50uSRZ6IFsuFq2UVErCz4I6tq/K1SZFPTOtr/KBHBeksoGMGw==";
  expected.blob_endpoint = std::make_unique<std::string>(
      "http://127.0.0.1:10000/devstoreaccount1");
  ASSERT_EQ(actual, expected);
}

const char *sValidAzuriteConnString =
    "DefaultEndpointsProtocol=http;AccountName=devstoreaccount1;AccountKey="
    "Eby8vdM02xNOcqFlqUwJPLlmEtlCDXJ1OUzFT50uSRZ6IFsuFq2UVErCz4I6tq/K1SZFPTOtr/"
    "KBHBeksoGMGw==;BlobEndpoint=http://localhost:10000/"
    "devstoreaccount1;QueueEndpoint=http://localhost:10001/devstoreaccount1;";

TEST_P(ConnectionStringTest, ParseAzuriteValidConnString) {
  ConnectionString actual;
  ASSERT_EQ(ParseConnectionString(
                &actual, sValidAzuriteConnString, bIsEmulatedStorage),
            0);
  ConnectionString expected;
  expected.account_name = "devstoreaccount1";
  expected.account_key = "Eby8vdM02xNOcqFlqUwJPLlmEtlCDXJ1OUzFT50uSRZ6IFsuFq2UVErCz4I6tq/"
      "K1SZFPTOtr/KBHBeksoGMGw==";
  expected.blob_endpoint = std::make_unique<std::string>(
      "http://localhost:10000/devstoreaccount1");
  ASSERT_EQ(actual, expected);
}

TEST_P(ConnectionStringTest, ParseIllFormedConnString) {
  ConnectionString actual;
  ASSERT_EQ(ParseConnectionString(
                &actual, sIllFormedConnString, bIsEmulatedStorage),
            -1);
}

TEST_P(ConnectionStringTest, ParseConnStringMissingAccountName) {
  ConnectionString actual;
  ASSERT_EQ(ParseConnectionString(
                &actual, sConnStringMissingAccountName, bIsEmulatedStorage),
            -1);
}

TEST_P(ConnectionStringTest, ParseConnStringMissingAccountKey) {
  ConnectionString actual;
  ASSERT_EQ(ParseConnectionString(
                &actual, sConnStringMissingAccountKey, bIsEmulatedStorage),
            -1);
}

TEST(ConnectionStringTest, ParseConnStringMissingBlobEndpoint) {
  ConnectionString actual;
  ConnectionString expected;
  expected.account_name = "devstoreaccount1";
  expected.account_key = "Eby8vdM02xNOcqFlqUwJPLlmEtlCDXJ1OUzFT50uSRZ6IFsuFq2UVErCz4I6tq/"
      "K1SZFPTOtr/KBHBeksoGMGw==";
  ASSERT_EQ(ParseConnectionString(
                &actual, sConnStringMissingBlobEndpoint, false),
            0);
  ASSERT_EQ(actual, expected);

  ASSERT_EQ(ParseConnectionString(
                &actual, sConnStringMissingBlobEndpoint, true),
            -1);
}
