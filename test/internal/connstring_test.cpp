#include <gtest/gtest.h>
#include <azure/core/url.hpp>
#include "../../src/util/connstring.hpp"

const char* sValidConnString =
	"AccountName=devstoreaccount1;"
	"AccountKey=Eby8vdM02xNOcqFlqUwJPLlmEtlCDXJ1OUzFT50uSRZ6IFsuFq2UVErCz4I6tq/K1SZFPTOtr/KBHBeksoGMGw==;"
	"DefaultEndpointsProtocol=http;"
	"BlobEndpoint=http://127.0.0.1:10000/devstoreaccount1;"
	"QueueEndpoint=http://127.0.0.1:10001/devstoreaccount1;"
	"TableEndpoint=http://127.0.0.1:10002/devstoreaccount1;";

const char* sIllFormedConnString = // same as above but without the terminating semicolon of the last key-value pair
	"AccountName=devstoreaccount1;"
	"AccountKey=Eby8vdM02xNOcqFlqUwJPLlmEtlCDXJ1OUzFT50uSRZ6IFsuFq2UVErCz4I6tq/K1SZFPTOtr/KBHBeksoGMGw==;"
	"DefaultEndpointsProtocol=http;"
	"BlobEndpoint=http://127.0.0.1:10000/devstoreaccount1;"
	"QueueEndpoint=http://127.0.0.1:10001/devstoreaccount1;"
	"TableEndpoint=http://127.0.0.1:10002/devstoreaccount1";

const char* sIncompleteConnString = // no AccountName
"AccountKey=Eby8vdM02xNOcqFlqUwJPLlmEtlCDXJ1OUzFT50uSRZ6IFsuFq2UVErCz4I6tq/K1SZFPTOtr/KBHBeksoGMGw==;"
"DefaultEndpointsProtocol=http;"
"BlobEndpoint=http://127.0.0.1:10000/devstoreaccount1;"
"QueueEndpoint=http://127.0.0.1:10001/devstoreaccount1;"
"TableEndpoint=http://127.0.0.1:10002/devstoreaccount1;";

TEST(ConnectionStringTest, ParseValidConnString)
{
	az::ConnectionString actual = az::ParseConnectionString(sValidConnString);
	az::ConnectionString expected
	{
		"devstoreaccount1",
		"Eby8vdM02xNOcqFlqUwJPLlmEtlCDXJ1OUzFT50uSRZ6IFsuFq2UVErCz4I6tq/K1SZFPTOtr/KBHBeksoGMGw==",
		Azure::Core::Url("http://127.0.0.1:10000/devstoreaccount1")
	};
	ASSERT_EQ(actual, expected);
}

TEST(ConnectionStringTest, ParseIllFormedConnString)
{
	ASSERT_THROW(
		{
			try
			{
				az::ParseConnectionString(sIllFormedConnString);
			}
			catch (const az::ParsingError& exc)
			{
				ASSERT_STREQ(exc.what(), "Ill-formed connection string");
				throw;
			}
		},
		az::ParsingError
	);
}

TEST(ConnectionStringTest, ParseIncompleteConnString)
{
	ASSERT_THROW(
		{
			try
			{
				az::ParseConnectionString(sIncompleteConnString);
			}
			catch (const az::ParsingError& exc)
			{
				ASSERT_STREQ(exc.what(), "Connection string is missing one of 'AccountName', 'AccountKey' or 'BlobEndpoint'");
				throw;
			}
		},
		az::ParsingError
	);
}
