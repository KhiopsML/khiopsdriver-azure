#include <gtest/gtest.h>

#include "azureplugin.hpp"
#include "returnval.hpp"
#include "fixtures/storage_test.hpp"

#include <iostream>
#include <sstream>

#include <boost/uuid/uuid.hpp>            // uuid class
#include <boost/uuid/uuid_generators.hpp> // generators
#include <boost/uuid/uuid_io.hpp>         // streaming operators etc.

using namespace std;
using namespace az;

int copyFile(const char *file_name_input, const char *file_name_output, int nBufferSize);
int copyFileWithFseek(const char *file_name_input, const char *file_name_output, int nBufferSize);
int copyFileWithAppend(const char *file_name_input, const char *file_name_output, int nBufferSize);
int removeFile(const char *filename);
int compareSize(const char *file_name_output, long long int filesize);

void launch_test(string sInputUrl, string sOutputUrl, string sLocalFilePath, size_t nBufferSize);

TEST_F(AdvancedStorageTest, End2EndTest_SingleFile_512KB_OK)
{
	launch_test(sBQSomePartFileUrl.c_str(), sOutputUrl.c_str(), sLocalFilePath.c_str(), 512 * 1024);
}

TEST_F(AdvancedStorageTest, End2EndTest_SingleFile_2MB_OK)
{
	launch_test(sBQSomePartFileUrl.c_str(), sOutputUrl.c_str(), sLocalFilePath.c_str(), 2 * 1024 * 1024);
}

TEST_F(AdvancedStorageTest, End2EndTest_SingleFile_512B_OK)
{
	/* use this particular file because it is short and buffer size triggers lots of read operations */
	launch_test(sBQShortPartFileUrl.c_str(), sOutputUrl.c_str(), sLocalFilePath.c_str(), 512);
}

TEST_F(AdvancedStorageTest, End2EndTest_MultipartBQFile_512KB_OK)
{
	launch_test(sBQFileUrl.c_str(), sOutputUrl.c_str(), sLocalFilePath.c_str(), 512 * 1024);
}

TEST_F(AdvancedStorageTest, End2EndTest_MultipartBQEmptyFile_512KB_OK)
{
	launch_test(sBQEmptyFileUrl.c_str(), sOutputUrl.c_str(), sLocalFilePath.c_str(), 512 * 1024);
}

TEST_F(AdvancedStorageTest, End2EndTest_MultipartSplitFile_512KB_OK)
{
	launch_test(sSplitFileUrl.c_str(), sOutputUrl.c_str(), sLocalFilePath.c_str(), 512 * 1024);
}

TEST_F(AdvancedStorageTest, End2EndTest_MultipartSubsplitFile_512KB_OK)
{
	launch_test(sMultisplitFileUrl.c_str(), sOutputUrl.c_str(), sLocalFilePath.c_str(), 512 * 1024);
}

void launch_test(string sInputUrl, string sOutputUrl, string sLocalFilePath, size_t nBufferSize)
{
	cout << "Scheme: " << driver_getScheme() << endl;
	cout << "Is read-only: " << driver_isReadOnly() << endl;
	ASSERT_EQ(driver_fileExists(sInputUrl.c_str()), nTrue) << "input file does not exist";
	size_t nInputFileSize = driver_getFileSize(sInputUrl.c_str());
	cout << "Size of " << sInputUrl << " is " << nInputFileSize << endl;

	const vector<pair<string, int (*)(const char*, const char*, int)>> CopyFunctions =
	{
		make_pair("standard", copyFile),
		make_pair("fseek", copyFileWithFseek),
		make_pair("append", copyFileWithAppend)
	};

	for (const auto& CopyFunction : CopyFunctions)
	{
		cout << "Copy (" << CopyFunction.first << ") " << sInputUrl << " to " << sOutputUrl << endl;
		ASSERT_EQ(CopyFunction.second(sInputUrl.c_str(), sOutputUrl.c_str(), nBufferSize), nSuccess) << "failed to copy file";
		ASSERT_EQ(compareSize(sOutputUrl.c_str(), nInputFileSize), nSuccess) << "input file and output file sizes are different";
		driver_remove(sOutputUrl.c_str());
		ASSERT_EQ(driver_fileExists(sOutputUrl.c_str()), nFalse) << "failed to remove newly created file";
	}

	cout << "Copy to local " << sInputUrl << " to " << sLocalFilePath << endl;
	ASSERT_EQ(driver_copyToLocal(sInputUrl.c_str(), sLocalFilePath.c_str()), nSuccess) << "failed to copy file";

	cout << "Copy from local " << sLocalFilePath << " to " << sOutputUrl << endl;
	ASSERT_EQ(driver_copyFromLocal(sLocalFilePath.c_str(), sOutputUrl.c_str()), nSuccess) << "failed to copy file";
	ASSERT_EQ(driver_fileExists(sOutputUrl.c_str()), nTrue) << sOutputUrl << " is missing";
}

// Copy file_name_input to file_name_output by steps of 1Kb
int copyFile(const char *file_name_input, const char *file_name_output, int nBufferSize)
{
	// Opens for read
	void *fileinput = driver_fopen(file_name_input, 'r');
	if (fileinput == NULL)
	{
		printf("error : %s : %s\n", file_name_input, driver_getlasterror());
		return nFailure;
	}

	int copy_status = nSuccess;
	void *fileoutput = driver_fopen(file_name_output, 'w');
	if (fileoutput == NULL)
	{
		printf("error : %s : %s\n", file_name_input, driver_getlasterror());
		copy_status = nFailure;
	}

	if (copy_status == nSuccess)
	{
		// Reads the file by steps of nBufferSize and writes to the output file at each step
		char *buffer = new char[nBufferSize + 1]();
		long long int sizeRead = nBufferSize;
		long long int sizeWrite;
		driver_fseek(fileinput, 0, SEEK_SET);
		while (sizeRead == nBufferSize && copy_status == nSuccess)
		{
			sizeRead = driver_fread(buffer, sizeof(char), nBufferSize, fileinput);
			if (sizeRead == -1)
			{
				copy_status = nFailure;
				printf("error while reading %s : %s\n", file_name_input, driver_getlasterror());
			}
			else
			{
				sizeWrite = driver_fwrite(buffer, sizeof(char), (size_t)sizeRead, fileoutput);
				if (sizeWrite == -1)
				{
					copy_status = nFailure;
					printf("error while writing %s : %s\n", file_name_output, driver_getlasterror());
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
int copyFileWithFseek(const char *file_name_input, const char *file_name_output, int nBufferSize)
{
	// Opens for read
	void *fileinput = driver_fopen(file_name_input, 'r');
	if (fileinput == NULL)
	{
		printf("error : %s : %s\n", file_name_input, driver_getlasterror());
		return nFailure;
	}

	int copy_status = nSuccess;
	void *fileoutput = driver_fopen(file_name_output, 'w');
	if (fileoutput == NULL)
	{
		printf("error : %s : %s\n", file_name_input, driver_getlasterror());
		copy_status = nFailure;
	}

	if (copy_status == nSuccess)
	{
		// Reads the file by steps of nBufferSize and writes to the output file at each step
		char *buffer = new char[nBufferSize+1]();
		long long int sizeRead = nBufferSize;
		long long int sizeWrite;
		int cummulativeRead = 0;
		driver_fseek(fileinput, 0, SEEK_SET);
		while (sizeRead == nBufferSize && copy_status == nSuccess)
		{
			driver_fseek(fileinput, cummulativeRead, SEEK_SET);
			sizeRead = driver_fread(buffer, sizeof(char), nBufferSize, fileinput);
			cummulativeRead += sizeRead;
			if (sizeRead == -1)
			{
				copy_status = nFailure;
				printf("error while reading %s : %s\n", file_name_input, driver_getlasterror());
			}
			else
			{
				sizeWrite = driver_fwrite(buffer, sizeof(char), (size_t)sizeRead, fileoutput);
				if (sizeWrite == -1)
				{
					copy_status = nFailure;
					printf("error while writing %s : %s\n", file_name_output, driver_getlasterror());
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
int copyFileWithAppend(const char *file_name_input, const char *file_name_output, int nBufferSize)
{
	// Make sure output file doesn't exist
	driver_remove(file_name_output);

	// Opens for read
	void *fileinput = driver_fopen(file_name_input, 'r');
	if (fileinput == NULL)
	{
		printf("error : %s : %s\n", file_name_input, driver_getlasterror());
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
		while (sizeRead == nBufferSize && copy_status == nSuccess)
		{
			sizeRead = driver_fread(buffer, sizeof(char), nBufferSize, fileinput);
			if (sizeRead == -1)
			{
				copy_status = nFailure;
				printf("error while reading %s : %s\n", file_name_input, driver_getlasterror());
			}
			else
			{
				void *fileoutput = driver_fopen(file_name_output, 'a');
				if (fileoutput == NULL)
				{
					printf("error : %s : %s\n", file_name_input, driver_getlasterror());
					copy_status = nFailure;
				}

				sizeWrite = driver_fwrite(buffer, sizeof(char), (size_t)sizeRead, fileoutput);
				if (sizeWrite == -1)
				{
					copy_status = nFailure;
					printf("error while writing %s : %s\n", file_name_output, driver_getlasterror());
				}

				int closeStatus = driver_fclose(fileoutput);
				if (closeStatus != 0) {
					copy_status = nFailure;
					printf("error while closing %s : %s\n", file_name_output, driver_getlasterror());
				}
			}
		}

		delete[](buffer);
	}
	driver_fclose(fileinput);
	return copy_status;
}

int compareSize(const char *file_name_output, long long int filesize)
{
	int compare_status = nSuccess;
	long long int filesize_output = driver_getFileSize(file_name_output);
	printf("size of %s is %lld\n", file_name_output, filesize_output);
	if (filesize_output != filesize)
	{
		printf("Sizes of input and output are different\n");
		compare_status = nFailure;
	}
	if (driver_fileExists(file_name_output))
	{
		printf("File %s exists\n", file_name_output);
	}
	else
	{
		printf("Something's wrong : %s is missing\n", file_name_output);
		compare_status = nFailure;
	}
	return compare_status;
}
