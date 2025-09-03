#include <gtest/gtest.h>

#include "azureplugin.hpp"
#include "returnval.hpp"
#include "fixtures/storage_test.hpp"

#include <iostream>
#include <sstream>
#include <tuple>

#include <boost/uuid/uuid.hpp>            // uuid class
#include <boost/uuid/uuid_generators.hpp> // generators
#include <boost/uuid/uuid_io.hpp>         // streaming operators etc.

using namespace std;
using namespace az;

int CopyFile(const char *file_name_input, const char *file_name_output, size_t nBufferSize);
int CopyFileWithFseek(const char *file_name_input, const char *file_name_output, size_t nBufferSize);
int CopyFileWithAppend(const char *file_name_input, const char *file_name_output, size_t nBufferSize);
int removeFile(const char *filename);
int compareSize(const char *file_name_output, long long int filesize);

void EndToEndTest_(string sInputUrl, string sOutputUrl, string sLocalFilePath, size_t nBufferSize);

INSTANTIATE_TEST_SUITE_P(BlobAndShare, EndToEndTest, testing::Values(StorageType::BLOB, StorageType::SHARE), EndToEndTest::FormatParam);

TEST_P(EndToEndTest, SingleFile512KB)
{
	if (GetParam() == StorageType::SHARE)
	{
		GTEST_SKIP() << "not operational for SHARE services";
	}
	EndToEndTest_(url.BQSomeFilePart(), url.RandomOutputFile(), sLocalFilePath, 512ULL * 1024);
}

TEST_P(EndToEndTest, SingleFile2MB)
{
	if (GetParam() == StorageType::SHARE)
	{
		GTEST_SKIP() << "not operational for SHARE services";
	}
	EndToEndTest_(url.BQSomeFilePart(), url.RandomOutputFile(), sLocalFilePath, 2ULL * 1024 * 1024);
}

TEST_P(EndToEndTest, SingleFile512B)
{
	if (GetParam() == StorageType::SHARE)
	{
		GTEST_SKIP() << "not operational for SHARE services";
	}
	/* use this particular file because it is short and buffer size triggers lots of read operations */
	EndToEndTest_(url.BQShortFilePart(), url.RandomOutputFile(), sLocalFilePath, 512ULL);
}

TEST_P(EndToEndTest, MultipartBQFile512KB)
{
	if (GetParam() == StorageType::SHARE)
	{
		GTEST_SKIP() << "not operational for SHARE services";
	}
	EndToEndTest_(url.BQFile(), url.RandomOutputFile(), sLocalFilePath, 512ULL * 1024);
}

TEST_P(EndToEndTest, MultipartBQEmptyFile512KB)
{
	if (GetParam() == StorageType::SHARE)
	{
		GTEST_SKIP() << "not operational for SHARE services";
	}
	EndToEndTest_(url.BQEmptyFile(), url.RandomOutputFile(), sLocalFilePath, 512ULL * 1024);
}

TEST_P(EndToEndTest, MultipartSplitFile512KB)
{
	if (GetParam() == StorageType::SHARE)
	{
		GTEST_SKIP() << "not operational for SHARE services";
	}
	EndToEndTest_(url.SplitFile(), url.RandomOutputFile(), sLocalFilePath, 512ULL * 1024);
}

TEST_P(EndToEndTest, MultipartSubsplitFile512KB)
{
	if (GetParam() == StorageType::SHARE)
	{
		GTEST_SKIP() << "not operational for SHARE services";
	}
	EndToEndTest_(url.MultisplitFile(), url.RandomOutputFile(), sLocalFilePath, 512ULL * 1024);
}

void EndToEndTest_(string sInputUrl, string sOutputUrl, string sLocalFilePath, size_t nBufferSize)
{
	cout << "Scheme: " << driver_getScheme() << endl;
	cout << "Is read-only: " << driver_isReadOnly() << endl;
	ASSERT_EQ(driver_fileExists(sInputUrl.c_str()), nTrue) << "input file does not exist";
	size_t nInputFileSize = driver_getFileSize(sInputUrl.c_str());
	cout << "Size of " << sInputUrl << " is " << nInputFileSize << endl;

	for (const auto& Copy : { CopyFile, CopyFileWithFseek, CopyFileWithAppend })
	{
		ASSERT_EQ(Copy(sInputUrl.c_str(), sOutputUrl.c_str(), nBufferSize), nSuccess) << "failed to copy file";
		ASSERT_EQ(compareSize(sOutputUrl.c_str(), nInputFileSize), nSuccess) << "input file and output file sizes are different";
		driver_remove(sOutputUrl.c_str());
		ASSERT_EQ(driver_fileExists(sOutputUrl.c_str()), nFalse) << "failed to remove newly created file";
	}

	cout << "Copy to local " << sInputUrl << " to " << sLocalFilePath << endl;
	ASSERT_EQ(driver_copyToLocal(sInputUrl.c_str(), sLocalFilePath.c_str()), nSuccess) << "failed to copy file";

	cout << "Copy from local " << sLocalFilePath << " to " << sOutputUrl << endl;
	ASSERT_EQ(driver_copyFromLocal(sLocalFilePath.c_str(), sOutputUrl.c_str()), nSuccess) << "failed to copy file";
	ASSERT_EQ(driver_fileExists(sOutputUrl.c_str()), nTrue) << sOutputUrl << " is missing";
	driver_remove(sOutputUrl.c_str());
	ASSERT_EQ(driver_fileExists(sOutputUrl.c_str()), nFalse) << "failed to remove newly created file";
}

// Copy file_name_input to file_name_output by steps of 1Kb
int CopyFile(const char *sInputFileUrl, const char *sOutputFileUrl, size_t nBufferSize)
{
	cout << "Standard copy of " << sInputFileUrl << " to " << sOutputFileUrl << endl;
	// Opens for read
	void *fileinput = driver_fopen(sInputFileUrl, 'r');
	if (fileinput == NULL)
	{
		printf("error : %s : %s\n", sInputFileUrl, driver_getlasterror());
		return nFailure;
	}

	int copy_status = nSuccess;
	void *fileoutput = driver_fopen(sOutputFileUrl, 'w');
	if (fileoutput == NULL)
	{
		printf("error : %s : %s\n", sInputFileUrl, driver_getlasterror());
		copy_status = nFailure;
	}

	if (copy_status == nSuccess)
	{
		// Reads the file by steps of nBufferSize and writes to the output file at each step
		char *buffer = new char[nBufferSize + 1]();
		long long int sizeRead = nBufferSize;
		long long int sizeWrite;
		driver_fseek(fileinput, 0, SEEK_SET);
		while (sizeRead == (long long int)nBufferSize && copy_status == nSuccess)
		{
			sizeRead = driver_fread(buffer, sizeof(char), nBufferSize, fileinput);
			if (sizeRead == -1)
			{
				copy_status = nFailure;
				printf("error while reading %s : %s\n", sInputFileUrl, driver_getlasterror());
			}
			else
			{
				sizeWrite = driver_fwrite(buffer, sizeof(char), (size_t)sizeRead, fileoutput);
				if (sizeWrite == -1)
				{
					copy_status = nFailure;
					printf("error while writing %s : %s\n", sOutputFileUrl, driver_getlasterror());
				}
			}
		}
		driver_fclose(fileoutput);
		delete[](buffer);
	}
	driver_fclose(fileinput);
	return copy_status;
}

// Copy file_name_input to file_name_output by steps of 1Kb by using fseek before each read
int CopyFileWithFseek(const char *sInputFileUrl, const char *sOutputFileUrl, size_t nBufferSize)
{
	cout << "FSeek copy of " << sInputFileUrl << " to " << sOutputFileUrl << endl;
	// Opens for read
	void *fileinput = driver_fopen(sInputFileUrl, 'r');
	if (fileinput == NULL)
	{
		printf("error : %s : %s\n", sInputFileUrl, driver_getlasterror());
		return nFailure;
	}

	int copy_status = nSuccess;
	void *fileoutput = driver_fopen(sOutputFileUrl, 'w');
	if (fileoutput == NULL)
	{
		printf("error : %s : %s\n", sInputFileUrl, driver_getlasterror());
		copy_status = nFailure;
	}

	if (copy_status == nSuccess)
	{
		// Reads the file by steps of nBufferSize and writes to the output file at each step
		char *buffer = new char[nBufferSize+1]();
		long long int sizeRead = nBufferSize;
		long long int sizeWrite;
		long long int cummulativeRead = 0;
		driver_fseek(fileinput, 0, SEEK_SET);
		while (sizeRead == (long long int)nBufferSize && copy_status == nSuccess)
		{
			driver_fseek(fileinput, cummulativeRead, SEEK_SET);
			sizeRead = driver_fread(buffer, sizeof(char), nBufferSize, fileinput);
			cummulativeRead += sizeRead;
			if (sizeRead == -1)
			{
				copy_status = nFailure;
				printf("error while reading %s : %s\n", sInputFileUrl, driver_getlasterror());
			}
			else
			{
				sizeWrite = driver_fwrite(buffer, sizeof(char), (size_t)sizeRead, fileoutput);
				if (sizeWrite == -1)
				{
					copy_status = nFailure;
					printf("error while writing %s : %s\n", sOutputFileUrl, driver_getlasterror());
				}
			}
		}
		driver_fclose(fileoutput);
		delete[](buffer);
	}
	driver_fclose(fileinput);
	return copy_status;
}

// Copy file_name_input to file_name_output by steps of 1Kb
int CopyFileWithAppend(const char *sInputFileUrl, const char *sOutputFileUrl, size_t nBufferSize)
{
	cout << "Append copy of " << sInputFileUrl << " to " << sOutputFileUrl << endl;
	// Make sure output file doesn't exist
	driver_remove(sOutputFileUrl);

	// Opens for read
	void *fileinput = driver_fopen(sInputFileUrl, 'r');
	if (fileinput == NULL)
	{
		printf("error : %s : %s\n", sInputFileUrl, driver_getlasterror());
		return nFailure;
	}

	int copy_status = nSuccess;

	if (copy_status == nSuccess)
	{
		// Reads the file by steps of nBufferSize and writes to the output file at each step
		char *buffer = new char[nBufferSize + 1]();
		long long int sizeRead = nBufferSize;
		long long int sizeWrite;
		driver_fseek(fileinput, 0, SEEK_SET);
		while (sizeRead == (long long int)nBufferSize && copy_status == nSuccess)
		{
			sizeRead = driver_fread(buffer, sizeof(char), nBufferSize, fileinput);
			if (sizeRead == -1)
			{
				copy_status = nFailure;
				printf("error while reading %s : %s\n", sInputFileUrl, driver_getlasterror());
			}
			else
			{
				void *fileoutput = driver_fopen(sOutputFileUrl, 'a');
				if (fileoutput == NULL)
				{
					printf("error : %s : %s\n", sInputFileUrl, driver_getlasterror());
					copy_status = nFailure;
				}

				sizeWrite = driver_fwrite(buffer, sizeof(char), (size_t)sizeRead, fileoutput);
				if (sizeWrite == -1)
				{
					copy_status = nFailure;
					printf("error while writing %s : %s\n", sOutputFileUrl, driver_getlasterror());
				}

				int closeStatus = driver_fclose(fileoutput);
				if (closeStatus != 0) {
					copy_status = nFailure;
					printf("error while closing %s : %s\n", sOutputFileUrl, driver_getlasterror());
				}
			}
		}

		delete[](buffer);
	}
	driver_fclose(fileinput);
	return copy_status;
}

int compareSize(const char *sOutputFileUrl, long long int nFilesize)
{
	int compare_status = nSuccess;
	long long int filesize_output = driver_getFileSize(sOutputFileUrl);
	printf("size of %s is %lld\n", sOutputFileUrl, filesize_output);
	if (filesize_output != nFilesize)
	{
		printf("Sizes of input and output are different\n");
		compare_status = nFailure;
	}
	if (driver_fileExists(sOutputFileUrl))
	{
		printf("File %s exists\n", sOutputFileUrl);
	}
	else
	{
		printf("Something's wrong : %s is missing\n", sOutputFileUrl);
		compare_status = nFailure;
	}
	return compare_status;
}
