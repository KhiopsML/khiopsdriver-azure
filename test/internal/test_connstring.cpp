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

INSTANTIATE_TEST_SUITE_P(EmulatedAndNotEmulatedStorage, ConnectionStringTest,
                         testing::Values(false, true));

TEST_P(ConnectionStringTest, ParseValidConnString) {
  khiops_driver_azure::connstr::ConnectionString actual;
  ASSERT_EQ(khiops_driver_azure::connstr::ConnectionString::ParseConnectionString(
                &actual, sValidConnString, bIsEmulatedStorage),
            0);
  khiops_driver_azure::connstr::ConnectionString expected(
      "devstoreaccount1",
      "Eby8vdM02xNOcqFlqUwJPLlmEtlCDXJ1OUzFT50uSRZ6IFsuFq2UVErCz4I6tq/"
      "K1SZFPTOtr/KBHBeksoGMGw==");
  expected.blobEndpointPtr = std::make_unique<Azure::Core::Url>(
      "http://127.0.0.1:10000/devstoreaccount1");
  ASSERT_EQ(actual, expected);
}

TEST_P(ConnectionStringTest, ParseValidConnStringWithoutTrailingSemicolon) {
  khiops_driver_azure::connstr::ConnectionString actual;
  ASSERT_EQ(khiops_driver_azure::connstr::ConnectionString::ParseConnectionString(
                &actual, sValidConnStringWithoutTrailingSemicolon,
                bIsEmulatedStorage),
            0);
  khiops_driver_azure::connstr::ConnectionString expected(
      "devstoreaccount1",
      "Eby8vdM02xNOcqFlqUwJPLlmEtlCDXJ1OUzFT50uSRZ6IFsuFq2UVErCz4I6tq/"
      "K1SZFPTOtr/KBHBeksoGMGw==");
  expected.blobEndpointPtr = std::make_unique<Azure::Core::Url>(
      "http://127.0.0.1:10000/devstoreaccount1");
  ASSERT_EQ(actual, expected);
}

const char *sValidAzuriteConnString =
    "DefaultEndpointsProtocol=http;AccountName=devstoreaccount1;AccountKey="
    "Eby8vdM02xNOcqFlqUwJPLlmEtlCDXJ1OUzFT50uSRZ6IFsuFq2UVErCz4I6tq/K1SZFPTOtr/"
    "KBHBeksoGMGw==;BlobEndpoint=http://localhost:10000/"
    "devstoreaccount1;QueueEndpoint=http://localhost:10001/devstoreaccount1;";

TEST_P(ConnectionStringTest, ParseAzuriteValidConnString) {
  khiops_driver_azure::connstr::ConnectionString actual;
  ASSERT_EQ(khiops_driver_azure::connstr::ConnectionString::ParseConnectionString(
                &actual, sValidAzuriteConnString, bIsEmulatedStorage),
            0);
  khiops_driver_azure::connstr::ConnectionString expected(
      "devstoreaccount1",
      "Eby8vdM02xNOcqFlqUwJPLlmEtlCDXJ1OUzFT50uSRZ6IFsuFq2UVErCz4I6tq/"
      "K1SZFPTOtr/KBHBeksoGMGw==");
  expected.blobEndpointPtr = std::make_unique<Azure::Core::Url>(
      "http://localhost:10000/devstoreaccount1");
  ASSERT_EQ(actual, expected);
}

TEST_P(ConnectionStringTest, ParseIllFormedConnString) {
  khiops_driver_azure::connstr::ConnectionString actual;
  ASSERT_EQ(khiops_driver_azure::connstr::ConnectionString::ParseConnectionString(
                &actual, sIllFormedConnString, bIsEmulatedStorage),
            -1);
}

TEST_P(ConnectionStringTest, ParseConnStringMissingAccountName) {
  khiops_driver_azure::connstr::ConnectionString actual;
  ASSERT_EQ(khiops_driver_azure::connstr::ConnectionString::ParseConnectionString(
                &actual, sConnStringMissingAccountName, bIsEmulatedStorage),
            -1);
}

TEST_P(ConnectionStringTest, ParseConnStringMissingAccountKey) {
  khiops_driver_azure::connstr::ConnectionString actual;
  ASSERT_EQ(khiops_driver_azure::connstr::ConnectionString::ParseConnectionString(
                &actual, sConnStringMissingAccountKey, bIsEmulatedStorage),
            -1);
}

TEST(ConnectionStringTest, ParseConnStringMissingBlobEndpoint) {
  khiops_driver_azure::connstr::ConnectionString actual;
  khiops_driver_azure::connstr::ConnectionString expected(
      "devstoreaccount1",
      "Eby8vdM02xNOcqFlqUwJPLlmEtlCDXJ1OUzFT50uSRZ6IFsuFq2UVErCz4I6tq/"
      "K1SZFPTOtr/KBHBeksoGMGw==");
  ASSERT_EQ(khiops_driver_azure::connstr::ConnectionString::ParseConnectionString(
                &actual, sConnStringMissingBlobEndpoint, false),
            0);
  ASSERT_EQ(actual, expected);

  ASSERT_EQ(khiops_driver_azure::connstr::ConnectionString::ParseConnectionString(
                &actual, sConnStringMissingBlobEndpoint, true),
            -1);
}
