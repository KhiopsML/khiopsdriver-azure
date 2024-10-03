#ifdef __CYGWIN__
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "azureplugin.h"
#include "azureplugin_internal.h"
#include "contrib/matching.h"

#include <algorithm>
#include <assert.h>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <limits.h>
#include <memory>
#include <sstream>

#include "spdlog/spdlog.h"

// Include the necessary SDK headers
#include <azure/core.hpp>
#include <azure/storage/blobs.hpp>
// Include to support file shares
#include <azure/storage/files/shares.hpp>

#include <azure/identity/default_azure_credential.hpp>

#include <boost/uuid/uuid.hpp>            // uuid class
#include <boost/uuid/uuid_generators.hpp> // generators
#include <boost/uuid/uuid_io.hpp>         // streaming operators etc.

using namespace azureplugin;

constexpr const char* version = "0.1.0";
constexpr const char* driver_name = "Azure driver";
constexpr const char* driver_scheme = "https";
constexpr long long preferred_buffer_size = 4 * 1024 * 1024;

int bIsConnected = kFalse;

// Add appropriate using namespace directives
using namespace Azure::Storage;
using namespace Azure::Storage::Blobs;
using namespace Azure::Storage::Files::Shares;
using namespace Azure::Identity;

// Secrets should be stored & retrieved from secure locations such as Azure::KeyVault. For
// convenience and brevity of samples, the secrets are retrieved from environment variables.
std::string GetEndpointUrl()
{
	return std::getenv("AZURE_STORAGE_ACCOUNT_URL");
}
std::string GetAccountName()
{
	return std::getenv("AZURE_STORAGE_ACCOUNT_NAME");
}
std::string GetAccountKey()
{
	return std::getenv("AZURE_STORAGE_ACCOUNT_KEY");
}
std::string GetConnectionString()
{
	return std::getenv("AZURE_STORAGE_CONNECTION_STRING");
}

// Global bucket name
std::string globalBucketName;

// Last error
std::string lastError;

StreamVec<Reader> active_reader_handles;
StreamVec<Writer> active_writer_handles;

enum class Service
{
	UNKNOWN,
	BLOB,
	SHARE
};

template <typename T> struct DriverResult
{
	Azure::Response<T> response_;
	bool success_{false};

	DriverResult() : response_{{}, nullptr} // Azure::Response only has an explicit contructor
	{
	}

	explicit DriverResult(Azure::Response<T>&& response, bool success)
	    : response_{std::move(response)}, success_{success}
	{
	}

	template <typename U> DriverResult<U> ConvertFailureTo()
	{
		DriverResult<U> to;
		to.response_.RawResponse = std::move(response_.RawResponse);
		return to;
	}

	const T& GetValue() const
	{
		return response_.Value;
	}

	T& GetValue()
	{
		return response_.Value;
	}

	T&& TakeValue()
	{
		return std::move(response_.Value);
	}

	const decltype(response_.RawResponse)& GetRawResponse() const
	{
		return response_.RawResponse;
	}

	Azure::Core::Http::HttpStatusCode GetStatusCode() const
	{
		return GetRawResponse()->GetStatusCode();
	}

	const std::string& GetReasonPhrase() const
	{
		return GetRawResponse()->GetReasonPhrase();
	}

	explicit operator bool() const
	{
		return success_;
	}
};

#define KH_AZ_CONNECTION_ERROR(err_val)                                                                                \
	if (kFalse == bIsConnected)                                                                                    \
	{                                                                                                              \
		LogError("Error: driver not connected.");                                                              \
		return (err_val);                                                                                      \
	}

#define RETURN_STATUS(x) return std::move((x)).status();

#define RETURN_STATUS_ON_ERROR(x)                                                                                      \
	if (!(x))                                                                                                      \
	{                                                                                                              \
		RETURN_STATUS((x));                                                                                    \
	}

#define ERROR_ON_NULL_ARG(arg, err_val)                                                                                \
	if (!(arg))                                                                                                    \
	{                                                                                                              \
		std::ostringstream oss;                                                                                \
		oss << "Error passing null pointer to " << __func__;                                                   \
		LogError(oss.str());                                                                                   \
		return (err_val);                                                                                      \
	}

void LogError(const std::string& msg)
{
	lastError = msg;
	spdlog::error(lastError);
}

void LogException(const std::string& msg, const std::string& what)
{
	std::ostringstream oss;
	oss << msg << " " << what;
	LogError(oss.str());
}

#define LogBadStatus(a, b)

template <typename T> void LogBadResult(const DriverResult<T>& result, const std::string& msg)
{
	std::ostringstream oss;
	oss << msg << ": " << result.GetReasonPhrase();
	LogError(oss.str());
}

template <typename Stream> StreamIt<Stream> FindHandle(void* h, StreamVec<Stream>& handles)
{
	return std::find_if(handles.begin(), handles.end(),
			    [h](const StreamPtr<Stream>& act_h_ptr)
			    { return h == static_cast<void*>(act_h_ptr.get()); });
}

StreamIt<Reader> FindReader(void* h)
{
	return FindHandle<Reader>(h, active_reader_handles);
}

StreamIt<Writer> FindWriter(void* h)
{
	return FindHandle<Writer>(h, active_writer_handles);
}

template <typename Stream> void EraseRemove(StreamIt<Stream> pos, StreamVec<Stream>& handles)
{
	*pos = std::move(handles.back());
	handles.pop_back();
}

// Definition of helper functions
/*
gc::StatusOr<long long int> DownloadFileRangeToBuffer(const std::string &bucket_name,
                                                      const std::string &object_name,
                                                      char *buffer,
                                                      std::int64_t start_range,
                                                      std::int64_t end_range)
{
    auto reader = client.ReadObject(bucket_name, object_name, gcs::ReadRange(start_range, end_range));
    if (!reader)
    {
        auto &o_status = reader.status();
        return gc::Status{o_status.code(), "Error while creating reading stream; " + o_status.message()};
    }

    reader.read(buffer, end_range - start_range);
    if (reader.bad())
    {
        auto &o_status = reader.status();
        return gc::Status{o_status.code(), "Error while creating reading stream; " + o_status.message()};
    }

    long long int num_read = static_cast<long long>(reader.gcount());
    spdlog::debug("read = {}", num_read);

    return num_read;
}

gc::StatusOr<long long> ReadBytesInFile(MultiPartFile &multifile, char *buffer, tOffset to_read)
{
    // Start at first usable file chunk
    // Advance through file chunks, advancing buffer pointer
    // Until last requested byte was read
    // Or error occured

    tOffset bytes_read{0};

    // Lookup item containing initial bytes at requested offset
    const auto &cumul_sizes = multifile.cumulativeSize_;
    const tOffset common_header_length = multifile.commonHeaderLength_;
    const std::string &bucket_name = multifile.bucketname_;
    const auto &filenames = multifile.filenames_;
    char *buffer_pos = buffer;
    tOffset &offset = multifile.offset_;
    const tOffset offset_bak = offset; // in case of irrecoverable error, leave the multifile in its starting state

    auto greater_than_offset_it = std::upper_bound(cumul_sizes.begin(), cumul_sizes.end(), offset);
    size_t idx = static_cast<size_t>(std::distance(cumul_sizes.begin(), greater_than_offset_it));

    spdlog::debug("Use item {} to read @ {} (end = {})", idx, offset, *greater_than_offset_it);

    auto read_range_and_update = [&](const std::string &filename, tOffset start, tOffset end) -> gc::Status
    {
        auto maybe_actual_read = DownloadFileRangeToBuffer(bucket_name, filename, buffer_pos,
                                                           static_cast<int64_t>(start), static_cast<int64_t>(end));
        if (!maybe_actual_read)
        {
            offset = offset_bak;
            RETURN_STATUS(maybe_actual_read);
        }

        tOffset actual_read = *maybe_actual_read;

        bytes_read += actual_read;
        buffer_pos += actual_read;
        offset += actual_read;

        if (actual_read < (end - start))
        {
            spdlog::debug("End of file encountered");
            to_read = 0;
        }
        else
        {
            to_read -= actual_read;
        }

        return {};
    };

    // first file read

    const tOffset file_start = (idx == 0) ? offset : offset - cumul_sizes[idx - 1] + common_header_length;
    const tOffset read_end = std::min(file_start + to_read, file_start + cumul_sizes[idx] - offset);

    gc::Status read_status = read_range_and_update(filenames[idx], file_start, read_end);

    // continue with the next files
    while (read_status.ok() && to_read)
    {
        // read the missing bytes in the next files as necessary
        idx++;
        const tOffset start = common_header_length;
        const tOffset end = std::min(start + to_read, start + cumul_sizes[idx] - cumul_sizes[idx - 1]);

        read_status = read_range_and_update(filenames[idx], start, end);
    }

    return read_status.ok() ? bytes_read : gc::StatusOr<long long>{read_status};
}
*/

std::unique_ptr<Azure::Core::Http::RawResponse> MakeRawHttpResponsePtr(Azure::Core::Http::HttpStatusCode code,
								       const std::string& reason)
{
	return std::unique_ptr<Azure::Core::Http::RawResponse>(new Azure::Core::Http::RawResponse(1, 0, code, reason));
}

template <typename T>
DriverResult<T> MakeDriverHttpFailure(Azure::Core::Http::HttpStatusCode code, const std::string& reason)
{
	return DriverResult<T>{Azure::Response<T>(T{}, MakeRawHttpResponsePtr(code, reason)), false};
}

template <typename T> DriverResult<T> MakeDriverFailureFromException(const Azure::Storage::StorageException& e)
{
	return MakeDriverHttpFailure<T>(e.StatusCode, e.ReasonPhrase);
}

template <typename T> DriverResult<T> MakeDriverFailureFromException(const std::exception& e)
{
	return MakeDriverHttpFailure<T>(Azure::Core::Http::HttpStatusCode::None, e.what());
}

template <typename T> DriverResult<T> MakeDriverSuccess(T value)
{
	return DriverResult<T>{Azure::Response<T>(std::move(value), nullptr), true};
}

struct ParseUriResult
{
	Service service;
	std::string bucket;
	std::string object;
};

// Parses URI in the following forms:
//  - when accessing a real cloud service:
//      https://myaccount.blob.core.windows.net/mycontainer/myblob.txt
//  - when a storage emulator like Azurite is used, the URI will have a different form:
//    http[s]://127.0.0.1:10000/myaccount/mycontainer/myblob.txt
// Note: file service URIs e.g. https://myaccount.file.core.windows.net/myshare/myfolder/myfile.txt
//       are not supported at this time...
DriverResult<ParseUriResult> ParseAzureUri(const std::string& azure_uri)
{
	auto compare_suffix = [](const std::string& full, const std::string& suffix)
	{
		const size_t full_length = full.length();
		const size_t suffix_length = suffix.length();
		return (full_length >= suffix_length &&
			full.compare(full_length - suffix_length, suffix_length, suffix) == 0);
	};

	const Azure::Core::Url parsed_uri(azure_uri);

	if (parsed_uri.GetScheme() != "https" && parsed_uri.GetScheme() != "http")
	{
		return MakeDriverHttpFailure<ParseUriResult>(Azure::Core::Http::HttpStatusCode::BadRequest,
							     "Invalid Azure URI");
	}

	size_t bkt_pos = 0;
	size_t obj_pos = parsed_uri.GetPath().find('/');
	Service service = Service::UNKNOWN;

	const std::string host = parsed_uri.GetHost();
	constexpr auto az_domain = ".core.windows.net";

	if (compare_suffix(host, az_domain))
	{
		spdlog::debug("Provided URI is a production one.");

		constexpr auto blob_domain = ".blob.core.windows.net";
		constexpr auto file_domain = ".file.core.windows.net";

		if (compare_suffix(host, blob_domain))
		{
			spdlog::debug("Provided URI is a blob one.");
			service = Service::BLOB;
		}
		else if (compare_suffix(host, file_domain))
		{
			spdlog::debug("Provided URI is a file one.");
			service = Service::SHARE;
		}
	}
	else
	{
		spdlog::debug("Provided URI is a testing one.");
		service = Service::BLOB;
		bkt_pos = obj_pos + 1;
		obj_pos = parsed_uri.GetPath().find('/', bkt_pos);
	}

	if (obj_pos == std::string::npos)
	{
		return MakeDriverHttpFailure<ParseUriResult>(Azure::Core::Http::HttpStatusCode::BadRequest,
							     "Invalid Azure URI, missing object name: " + azure_uri);
	}

	ParseUriResult res{service, parsed_uri.GetPath().substr(bkt_pos, obj_pos - bkt_pos),
			   parsed_uri.GetPath().substr(obj_pos + 1)};

	return MakeDriverSuccess<ParseUriResult>(std::move(res));
}

DriverResult<ParseUriResult> GetServiceBucketAndObjectNames(const char* sFilePathName)
{
	auto maybe_parse_res = ParseAzureUri(sFilePathName);

	if (maybe_parse_res)
	{
		const ParseUriResult& val = maybe_parse_res.GetValue();
		spdlog::debug("Bucket: {}, Object: {}", val.bucket, val.object);
	}

	// std::cout << "Bucket: " << val.bucket << ", Object: " << val.object << "\n";
	/*
    if (!maybe_parse_res)
    {
        return maybe_parse_res;
    }

    // fallback to default bucket if bucket empty
    if (maybe_parse_res->bucket.empty())
    {
        if (globalBucketName.empty())
        {
            maybe_parse_res = gc::Status{gc::StatusCode::kInternal, "No bucket specified and GCS_BUCKET_NAME is not set!"};
        }
        else
        {
            maybe_parse_res->bucket = globalBucketName;
        }
    }
    */
	return maybe_parse_res;
}

std::string ToLower(const std::string& str)
{
	std::string low{str};
	const size_t cnt = low.length();
	for (size_t i = 0; i < cnt; i++)
	{
		low[i] = static_cast<char>(std::tolower(
		    static_cast<unsigned char>(low[i]))); // see https://en.cppreference.com/w/cpp/string/byte/tolower
	}
	return low;
}

std::string GetEnvironmentVariableOrDefault(const std::string& variable_name, const std::string& default_value)
{
	char* value = std::getenv(variable_name.c_str());

	if (value && std::strlen(value) > 0)
	{
		return value;
	}

	const std::string low_key = ToLower(variable_name);
	if (low_key.find("token") || low_key.find("password") || low_key.find("key") || low_key.find("secret"))
	{
		spdlog::debug("No {} specified, using **REDACTED** as default.", variable_name);
	}
	else
	{
		spdlog::debug("No {} specified, using '{}' as default.", variable_name, default_value);
	}

	return default_value;
}

template <typename ServiceClientType> ServiceClientType GetServiceClient()
{
	// TODO Should allow different auth options like described in: https://learn.microsoft.com/en-us/azure/storage/blobs/authorize-data-operations-cli
	const std::string connectionString =
	    GetEnvironmentVariableOrDefault("AZURE_STORAGE_CONNECTION_STRING",
					    "DefaultEndpointsProtocol=http;AccountName=devstoreaccount1;AccountKey="
					    "Eby8vdM02xNOcqFlqUwJPLlmEtlCDXJ1OUzFT50uSRZ6IFsuFq2UVErCz4I6tq/K1SZFPTOtr/"
					    "KBHBeksoGMGw==;BlobEndpoint=http://127.0.0.1:10000/devstoreaccount1;");
	return ServiceClientType::CreateFromConnectionString(connectionString);
}

bool WillSizeCountProductOverflow(size_t size, size_t count)
{
	constexpr size_t max_prod_usable{static_cast<size_t>(std::numeric_limits<tOffset>::max())};
	return (max_prod_usable / size < count || max_prod_usable / count < size);
}

/*
// pre condition: stream is of a writing type. do not call otherwise.
gc::Status CloseWriterStream(Handle &stream)
{
    gc::StatusOr<gcs::ObjectMetadata> maybe_meta;
    std::ostringstream err_msg_os;

    // close the stream to flush all remaining bytes in the put area
    auto &writer = stream.GetWriter().writer_;
    writer.Close();
    maybe_meta = writer.metadata();
    if (!maybe_meta)
    {
        err_msg_os << "Error during upload";
    }
    else if (HandleType::kAppend == stream.type)
    {
        // the tmp file is valid and ready for composition with the source
        const auto &writer_h = stream.GetWriter();
        const std::string &bucket = writer_h.bucketname_;
        const std::string &append_source = writer_h.filename_;
        const std::string &dest = writer_h.append_target_;
        std::vector<gcs::ComposeSourceObject> source_objects = {{dest, {}, {}}, {append_source, {}, {}}};
        maybe_meta = client.ComposeObject(bucket, std::move(source_objects), dest);

        // whatever happened, delete the tmp file
        gc::Status delete_status = client.DeleteObject(bucket, append_source);

        // TODO: what to do with an error on Delete?
        (void)delete_status;

        // if composition failed, nothing is written, the source did not change. signal it
        if (!maybe_meta)
        {
            err_msg_os << "Error while uploading the data to append";
        }
    }

    if (maybe_meta)
    {
        return {};
    }

    const gc::Status &status = maybe_meta.status();
    err_msg_os << ": " << maybe_meta.status().message();
    return gc::Status{status.code(), err_msg_os.str()};
}
*/
// Implementation of driver functions

// Get from a container a list of blobs matching a name pattern.
// To get a limited list of blobs to filter per request, the request includes a well defined
// prefix contained in the pattern
using BlobItems = std::vector<Azure::Storage::Blobs::Models::BlobItem>;
DriverResult<BlobItems> FilterList(const std::string& bucket, const std::string& pattern,
				   size_t pattern_1st_sp_char_pos)
{
	using value_t = BlobItems;

	std::vector<Azure::Storage::Blobs::Models::BlobItem> res;

	auto container_client = GetServiceClient<BlobServiceClient>().GetBlobContainerClient(bucket);
	ListBlobsOptions options;
	options.Prefix = pattern.substr(0, pattern_1st_sp_char_pos);
	try
	{
		for (auto blobs_page = container_client.ListBlobs(options); blobs_page.HasPage();
		     blobs_page.MoveToNextPage())
		{
			// filter blobs by match with pattern
			for (auto& item : blobs_page.Blobs)
			{
				if (!item.IsDeleted && utils::gitignore_glob_match(item.Name, pattern))
				{
					res.emplace_back(std::move(item));
				}
			}
		}
		if (res.empty())
		{
			return MakeDriverHttpFailure<value_t>(Azure::Core::Http::HttpStatusCode::NotFound,
							      "No blob matching pattern in container.");
		}
		return MakeDriverSuccess<value_t>(std::move(res));
	}
	catch (const Azure::Storage::StorageException& e)
	{
		return MakeDriverFailureFromException<value_t>(e);
	}
	catch (const std::exception& e)
	{
		return MakeDriverFailureFromException<value_t>(e);
	}
}

const char* driver_getDriverName()
{
	return driver_name;
}

const char* driver_getVersion()
{
	return version;
}

const char* driver_getScheme()
{
	return driver_scheme;
}

int driver_isReadOnly()
{
	return kFalse;
}

int driver_connect()
{
	const std::string loglevel = GetEnvironmentVariableOrDefault("AZURE_DRIVER_LOGLEVEL", "info");
	if (loglevel == "debug")
		spdlog::set_level(spdlog::level::debug);
	else if (loglevel == "trace")
		spdlog::set_level(spdlog::level::trace);
	else
		spdlog::set_level(spdlog::level::info);

	spdlog::debug("Connect {}", loglevel);

	// Initialize variables from environment
	globalBucketName = GetEnvironmentVariableOrDefault("AZURE_BUCKET_NAME", "");

	// Tester la connexion
	const auto blobServiceClient = GetServiceClient<BlobServiceClient>();
	try
	{
		blobServiceClient.GetProperties();
		spdlog::debug("Connexion valide.");
		bIsConnected = kTrue;
		return kSuccess;
	}
	catch (const std::exception& e)
	{
		LogException("Connection error.", e.what());
		return kFailure;
	}
}

int driver_disconnect()
{
	// clear handles
	active_reader_handles.clear();
	active_writer_handles.clear();

	// manage internal state
	bIsConnected = kFalse;
	return kSuccess;
}

int driver_isConnected()
{
	return bIsConnected;
}

long long int driver_getSystemPreferredBufferSize()
{
	return preferred_buffer_size; // 4 Mo
}

int driver_exist(const char* filename)
{
	KH_AZ_CONNECTION_ERROR(kFalse);

	ERROR_ON_NULL_ARG(filename, kFalse);

	spdlog::debug("exist {}", filename);

	std::string file_uri = filename;
	spdlog::debug("exist file_uri {}", file_uri);
	spdlog::debug("exist last char {}", file_uri.back());

	if (file_uri.back() == '/')
	{
		return driver_dirExists(filename);
	}
	else
	{
		return driver_fileExists(filename);
	}
}

#define RETURN_ON_ERROR(driver_result, msg, err_val)                                                                   \
	if (!(driver_result))                                                                                          \
	{                                                                                                              \
		LogBadResult((driver_result), (msg));                                                                  \
		return (err_val);                                                                                      \
	}

#define ERROR_ON_NAMES(names_result, err_val) RETURN_ON_ERROR((names_result), "Error parsing URL", (err_val))

Azure::Nullable<size_t> FindPatternSpecialChar(const std::string& pattern)
{
	spdlog::debug("Parse multifile pattern {}", pattern);

	constexpr auto special_chars = "*?![^";

	size_t from_offset = 0;
	size_t found_at = pattern.find_first_of(special_chars, from_offset);
	while (found_at != std::string::npos)
	{
		const char found = pattern[found_at];
		spdlog::debug("special char {} found at {}", found, found_at);

		if (found_at > 0 && pattern[found_at - 1] == '\\')
		{
			spdlog::debug("preceded by a \\, so not so special");
			from_offset = found_at + 1;
			found_at = pattern.find_first_of(special_chars, from_offset);
		}
		else
		{
			spdlog::debug("not preceded by a \\, so really a special char");
			break;
		}
	}
	return found_at != std::string::npos ? found_at : Azure::Nullable<size_t>{};
}

int driver_fileExists(const char* sFilePathName)
{
	KH_AZ_CONNECTION_ERROR(kFalse);

	ERROR_ON_NULL_ARG(sFilePathName, kFalse);

	spdlog::debug("fileExist {}", sFilePathName);

	auto maybe_parsed_names = ParseAzureUri(sFilePathName);
	if (!maybe_parsed_names)
	{
		std::ostringstream oss;
		oss << "Error while parsing Uri. " << maybe_parsed_names.GetReasonPhrase();
		LogError(oss.str());
	}

	const auto& val = maybe_parsed_names.GetValue();
	if (Service::BLOB != val.service)
	{
		LogError("Error checking blob's existence: not a URL of a blob service.");
		return kFalse;
	}

	const auto pattern_1st_sp_char_pos = FindPatternSpecialChar(val.object);
	if (!pattern_1st_sp_char_pos)
	{
		const auto& blob_client =
		    GetServiceClient<BlobServiceClient>().GetBlobContainerClient(val.bucket).GetBlobClient(val.object);
		try
		{
			// GetProperties throws in case of error, even if the error is NotFound.
			blob_client.GetProperties();
			spdlog::debug("file {} exists.", sFilePathName);
			return kTrue;
		}
		catch (const Azure::Storage::StorageException& e)
		{
			if (e.StatusCode == Azure::Core::Http::HttpStatusCode::NotFound)
			{
				spdlog::debug("File not found. {}", e.what());
			}
			else
			{
				LogException("Error while checking file's presence.", e.what());
			}
			return kFalse;
		}
		catch (const std::exception& e)
		{
			LogException("Error while checking file's presence.", e.what());
			return kFalse;
		}
	}

	const auto filter_res = FilterList(val.bucket, val.object, *pattern_1st_sp_char_pos);
	if (!filter_res)
	{
		if (spdlog::get_level() >= spdlog::level::debug)
		{
			if (Azure::Core::Http::HttpStatusCode::NotFound == filter_res.GetStatusCode())
			{
				spdlog::debug(filter_res.GetReasonPhrase());
			}
			else
			{
				LogBadResult<BlobItems>(filter_res, "Error while listing blobs in container.");
			}
		}
		return kFalse;
	}

	return kTrue;
}

int driver_dirExists(const char* sFilePathName)
{
	KH_AZ_CONNECTION_ERROR(kFalse);

	ERROR_ON_NULL_ARG(sFilePathName, kFalse);

	spdlog::debug("dirExist {}", sFilePathName);
	return kTrue;
}

DownloadBlobToOptions MakeDlBlobOptions(int64_t range_start, Azure::Nullable<int64_t> range_length = {})
{
	Azure::Core::Http::HttpRange range{range_start, range_length};
	DownloadBlobToOptions opt;
	opt.Range = std::move(range);
	return opt;
}

// this function rethrows the exception thrown by Azure's API if an error occurs.
size_t ReadPart(const BlobClient& blob_client, std::vector<uint8_t>& dest, int64_t offset)
{
	const size_t to_read = dest.size();
	const auto dl_options = MakeDlBlobOptions(offset, static_cast<int64_t>(to_read));
	const auto dl_res = blob_client.DownloadTo(dest.data(), to_read, dl_options);
	return static_cast<size_t>(*(dl_res.Value.ContentRange.Length));
}

std::vector<uint8_t> ReadPart(const BlobClient& blob_client, int64_t offset, size_t to_read)
{
	std::vector<uint8_t> dest(to_read);
	const size_t bytes_read = ReadPart(blob_client, dest, offset);
	if (bytes_read < to_read)
	{
		dest.resize(bytes_read);
	}
	return dest;
}

Azure::Nullable<std::vector<uint8_t>> FindHeader(const BlobClient& blob_client)
{
	constexpr size_t block_size{10 * 1024 * 1024};
	std::vector<uint8_t> header(block_size);

	// read by blocks until new line char is found or end of stream
	int64_t bytes_read = 0;

	bool found = false;
	bool exhausted = false;

	while (!exhausted)
	{
		const auto new_data_start = header.data() + bytes_read;
		const auto dl_options = MakeDlBlobOptions(bytes_read, block_size);
		const auto dl_res = blob_client.DownloadTo(new_data_start, block_size, dl_options);
		const auto dl_bytes_read = *(dl_res.Value.ContentRange.Length);

		const auto search_end = new_data_start + dl_bytes_read;
		const auto new_line_pos = std::find(new_data_start, search_end, static_cast<uint8_t>('\n'));

		if (new_line_pos != search_end)
		{
			// found it!
			// trim the vector containing the header
			found = true;
			header.resize(std::distance(header.data(), new_line_pos) + 1u);
			break;
		}

		bytes_read += dl_bytes_read;

		exhausted = bytes_read == dl_res.Value.BlobSize;

		if (!exhausted)
		{
			header.resize(header.size() + block_size);
		}
	}

	return found ? std::move(header) : Azure::Nullable<std::vector<uint8_t>>{};
}

// Azure::Nullable<std::vector<uint8_t>> FindHeader(const Azure::Storage::Blobs::Models::DownloadBlobResult& dl_blob)
// {
//     constexpr size_t block_size{10*1024*1024};
//     const size_t blob_size = static_cast<size_t>(dl_blob.BlobSize);

//     std::vector<uint8_t> header;
//     const size_t to_read = std::min(blob_size, block_size);

//     // read by blocks until new line char is found
//     size_t bytes_read = 0;
//     auto& stream_ptr = dl_blob.BodyStream;

//     bool found = false;
//     while (bytes_read < blob_size)
//     {
//         // the API contract is that ReadToCount reads up to size or end of stream
//         header.resize(header.size() + to_read);
//         const size_t part_read = stream_ptr->ReadToCount(header.data()+bytes_read, to_read);
//         const auto search_start_it = header.cbegin()+static_cast<ptrdiff_t>(bytes_read);
//         const auto search_end_it = search_start_it+static_cast<ptrdiff_t>(part_read);
//         const auto new_line_it = std::find(search_start_it, search_end_it, static_cast<uint8_t>('\n'));

//         if (new_line_it != search_end_it)
//         {
//             // found it!
//             // trim the vector containing the header
//             found = true;
//             header.erase(new_line_it+1,header.cend());
//             header.shrink_to_fit();
//             break;
//         }

//         bytes_read += part_read;
//     }

//     return found ? std::move(header) : Azure::Nullable<std::vector<uint8_t>>{};
// }

bool IsSameHeader(const BlobContainerClient& container_client, const Blobs::Models::BlobItem& blob_item,
		  const std::vector<uint8_t>& header, std::vector<uint8_t>& part_buffer)
{
	const auto blob_client = container_client.GetBlobClient(blob_item.Name);
	ReadPart(blob_client, part_buffer, 0);
	return part_buffer == header;
}

DriverResult<long long> GetFileSize(const ParseUriResult& parsed_names)
{
	using value_t = long long;

	const auto container_client = GetServiceClient<BlobServiceClient>().GetBlobContainerClient(parsed_names.bucket);

	// if single file, get it, else multifile pattern, list files
	const std::string& object = parsed_names.object;
	const auto pattern_1st_sp_char_pos = FindPatternSpecialChar(object);

	if (!pattern_1st_sp_char_pos)
	{
		// get the property directly
		const auto blob_client = container_client.GetBlobClient(object);
		try
		{
			const auto props = blob_client.GetProperties();
			value_t result = static_cast<value_t>(props.Value.BlobSize);
			return MakeDriverSuccess<value_t>(result);
		}
		catch (const Azure::Storage::StorageException& e)
		{
			return MakeDriverFailureFromException<value_t>(e);
		}
		catch (const std::exception& e)
		{
			return MakeDriverFailureFromException<value_t>(e);
		}
	}

	//multifile, list the files corresponding to the pattern
	auto filter_res = FilterList(parsed_names.bucket, object, *pattern_1st_sp_char_pos);
	if (!filter_res)
	{
		return filter_res.ConvertFailureTo<value_t>();
	}

	const auto& blobs_list = filter_res.GetValue();

	// if single file, short treatment
	if (blobs_list.size() == 1)
	{
		return MakeDriverSuccess(static_cast<long long>(blobs_list[0].BlobSize));
	}

	// several files
	// get the sizes and read the headers to identify common headers if any
	// first file
	const auto first_blob_client = container_client.GetBlobClient(blobs_list[0].Name);
	try
	{
		const auto maybe_header = FindHeader(first_blob_client);

		if (!maybe_header)
		{
			return MakeDriverHttpFailure<long long>(Azure::Core::Http::HttpStatusCode::None,
								"Error while reading header of first file.");
		}

		const std::vector<uint8_t>& header = *maybe_header;
		const size_t header_size = header.size();

		// next files
		const size_t blob_count = blobs_list.size();
		long long total_size = blobs_list[0].BlobSize;
		bool same_headers = true;
		std::vector<uint8_t> part_buffer(header_size);

		for (size_t i = 1; i < blob_count; i++)
		{
			const auto& blob_item = blobs_list[i];
			total_size += static_cast<long long>(blob_item.BlobSize);

			// read the beginning of the file to check header if still necessary
			if (same_headers)
			{
				same_headers = IsSameHeader(container_client, blob_item, header, part_buffer);
			}
		}

		long long result =
		    same_headers ? total_size - static_cast<long long>((blob_count - 1) * header_size) : total_size;

		return MakeDriverSuccess<long long>(result);
	}
	catch (const Azure::Storage::StorageException& e)
	{
		return MakeDriverFailureFromException<long long>(e);
	}
	catch (const std::exception& e)
	{
		return MakeDriverFailureFromException<long long>(e);
	}
}

long long int driver_getFileSize(const char* filename)
{
	KH_AZ_CONNECTION_ERROR(kBadSize);

	ERROR_ON_NULL_ARG(filename, kBadSize);

	spdlog::debug("getFileSize {}", filename);

	const auto maybe_parsed_names = ParseAzureUri(filename);
	ERROR_ON_NAMES(maybe_parsed_names, kBadSize);

	const auto& parsed_names = maybe_parsed_names.GetValue();

	if (Service::BLOB != parsed_names.service)
	{
		LogError("Functionality not implemented for this type of service.");
		return kBadSize;
	}

	// if (val.service == SHARE) {
	//     auto props = GetShareServiceClient().GetShareClient(val.bucket)
	//             .GetRootDirectoryClient().GetFileClient(val.object)
	//             .GetProperties();
	//     return props.Value.FileSize;

	const auto res = GetFileSize(parsed_names);
	if (!res)
	{
		LogBadResult(res, "Error while getting file size.");
		return kBadSize;
	}

	return res.GetValue();
}

DriverResult<long long> ReadBytesInFile(Reader& multifile, unsigned char* buffer, tOffset to_read)
{
	// Start at first usable file chunk
	// Advance through file chunks, advancing buffer pointer
	// Until last requested byte was read
	// Or error occured

	const auto& cumul_sizes = multifile.cumulativeSize_;
	const tOffset common_header_length = multifile.commonHeaderLength_;
	const auto& filenames = multifile.filenames_;
	unsigned char* buffer_pos = buffer;
	tOffset& offset = multifile.offset_;
	const tOffset offset_bak = offset; // in case of irrecoverable error, leave the multifile in its starting state

	auto recover_offset = [&]() { offset = offset_bak; };

	auto read_range_and_update =
	    [&](const BlobContainerClient& container, const std::string& filename, tOffset start, tOffset length)
	{
		const auto blob_client = container.GetBlobClient(filename);
		const auto dl_options = MakeDlBlobOptions(static_cast<int64_t>(start), static_cast<int64_t>(length));
		blob_client.DownloadTo(static_cast<uint8_t*>(buffer_pos), static_cast<size_t>(length), dl_options);

		offset += length;
		buffer_pos += length;
		to_read -= length;
	};

	// Lookup item containing initial bytes at requested offset
	auto greater_than_offset_it = std::upper_bound(cumul_sizes.begin(), cumul_sizes.end(), offset);
	size_t idx = static_cast<size_t>(std::distance(cumul_sizes.begin(), greater_than_offset_it));

	spdlog::debug("Use item {} to read @ {} (end = {})", idx, offset, *greater_than_offset_it);

	const auto container_client =
	    GetServiceClient<BlobServiceClient>().GetBlobContainerClient(multifile.bucketname_);

	// first file read
	const tOffset file_start = (idx == 0) ? offset : offset - cumul_sizes[idx - 1] + common_header_length;
	const tOffset read_length = std::min(to_read, cumul_sizes[idx] - offset);

	try
	{
		read_range_and_update(container_client, filenames[idx], file_start, read_length);
	}
	catch (const StorageException& e)
	{
		recover_offset();
		return MakeDriverFailureFromException<long long>(e);
	}
	catch (const std::exception& e)
	{
		recover_offset();
		return MakeDriverFailureFromException<long long>(e);
	}

	// continue with the next files
	while (to_read)
	{
		// read the missing bytes in the next files as necessary
		idx++;
		const tOffset start = common_header_length;
		const tOffset length = std::min(to_read, cumul_sizes[idx] - cumul_sizes[idx - 1]);
		try
		{
			read_range_and_update(container_client, filenames[idx], start, length);
		}
		catch (const StorageException& e)
		{
			recover_offset();
			return MakeDriverFailureFromException<long long>(e);
		}
		catch (const std::exception& e)
		{
			recover_offset();
			return MakeDriverFailureFromException<long long>(e);
		}
	}

	return MakeDriverSuccess<long long>(to_read);
}

DriverResult<ReaderPtr> MakeReaderPtr(std::string bucket, std::string object)
{
	using value_t = ReaderPtr;

	auto make_simple_reader = [](std::string&& bucket, std::string&& object, std::string&& filename,
				     long long blob_size) -> ReaderPtr
	{
		std::vector<std::string> filenames;
		filenames.emplace_back(std::move(filename));
		std::vector<long long> cumulative_sizes = {blob_size};
		return std::make_unique<Reader>(std::move(bucket), std::move(object), 0, 0, std::move(filenames),
						std::move(cumulative_sizes));
	};

	const auto container_client = GetServiceClient<BlobServiceClient>().GetBlobContainerClient(bucket);

	// if single file, get it, else multifile pattern, list files
	const auto pattern_1st_sp_char_pos = FindPatternSpecialChar(object);

	if (!pattern_1st_sp_char_pos)
	{
		auto blob_client = container_client.GetBlobClient(object);
		try
		{
			const auto props_response = blob_client.GetProperties();
			std::string filename = object;
			return MakeDriverSuccess<value_t>(
			    make_simple_reader(std::move(bucket), std::move(object), std::move(filename),
					       static_cast<long long>(props_response.Value.BlobSize)));
		}
		catch (const Azure::Storage::StorageException& e)
		{
			return MakeDriverHttpFailure<value_t>(e.StatusCode, e.ReasonPhrase);
		}
		catch (const std::exception& e)
		{
			return MakeDriverHttpFailure<value_t>(Azure::Core::Http::HttpStatusCode::None, e.what());
		}
	}

	//multifile, list the files corresponding to the pattern
	auto filter_res = FilterList(bucket, object, *pattern_1st_sp_char_pos);
	if (!filter_res)
	{
		return filter_res.ConvertFailureTo<value_t>();
	}

	auto& blobs_list = filter_res.GetValue();

	// if single file, short treatment
	if (blobs_list.size() == 1)
	{
		auto& item = blobs_list[0];
		return MakeDriverSuccess<value_t>(make_simple_reader(
		    std::move(bucket), std::move(object), std::move(item.Name), static_cast<long long>(item.BlobSize)));
	}

	// several files, gather data about names and sizes
	const size_t blob_count = blobs_list.size();
	std::vector<std::string> filenames;
	filenames.reserve(blob_count);
	std::vector<long long> cumulativeSize;
	cumulativeSize.reserve(blob_count);

	const auto& first_blob = blobs_list[0];
	const auto first_blob_client = container_client.GetBlobClient(first_blob.Name);
	try
	{
		const auto maybe_header = FindHeader(first_blob_client);

		if (!maybe_header)
		{
			return MakeDriverHttpFailure<value_t>(Azure::Core::Http::HttpStatusCode::None,
							      "Error while reading header of first file.");
		}

		const std::vector<uint8_t>& header = *maybe_header;
		const size_t header_size = header.size();
		filenames.emplace_back(first_blob.Name);
		cumulativeSize.push_back(static_cast<long long>(blobs_list[0].BlobSize));

		// next files
		bool same_headers = true;
		std::vector<uint8_t> part_buffer(header_size);

		for (size_t i = 1; i < blob_count; i++)
		{
			const auto& curr_blob_item = blobs_list[i];
			filenames.emplace_back(curr_blob_item.Name);
			cumulativeSize.push_back(cumulativeSize.back() +
						 static_cast<long long>(curr_blob_item.BlobSize));

			// read the beginning of the file to check header if still necessary
			if (same_headers)
			{
				same_headers = IsSameHeader(container_client, curr_blob_item, header, part_buffer);
			}
		}

		if (same_headers)
		{
			// update cumulativeSize
			for (size_t i = 1; i < blob_count; i++)
			{
				cumulativeSize[i] -= static_cast<long long>(i * header_size);
			}
		}

		return MakeDriverSuccess<value_t>(std::make_unique<Reader>(
		    std::move(bucket), std::move(object), 0, static_cast<long long>(header_size), std::move(filenames),
		    std::move(cumulativeSize)));
	}
	catch (const Azure::Storage::StorageException& e)
	{
		return MakeDriverFailureFromException<value_t>(e);
	}
	catch (const std::exception& e)
	{
		return MakeDriverFailureFromException<value_t>(e);
	}
}

DriverResult<WriterPtr> MakeWriterPtr(std::string bucket, std::string object)
{
    using value_t = WriterPtr;

    const auto container_client = GetServiceClient<BlobServiceClient>().GetBlobContainerClient(bucket);
	try
    {
        auto writer_ptr = std::make_unique<Writer>(std::move(bucket), std::move(object), container_client.GetBlockBlobClient(object));
        return MakeDriverSuccess<value_t>(std::move(writer_ptr));
    }
    catch(const std::exception& e)
    {
        return MakeDriverFailureFromException<value_t>(e);
    }
}

// This template is only here to get specialized
template <typename Stream> Stream* PushBackHandle(StreamPtr<Stream>&& stream_ptr, StreamVec<Stream>& handles)
{
	Stream* res = stream_ptr.get();
	handles.push_back(std::move(stream_ptr));
	return res;
}

template <typename Stream>
DriverResult<Stream*>
RegisterStream(std::function<DriverResult<StreamPtr<Stream>>(std::string, std::string)> MakeStreamPtr,
	       std::string&& bucket, std::string&& object, StreamVec<Stream>& streams)
{
	auto maybe_stream_ptr = MakeStreamPtr(std::move(bucket), std::move(object));
	if (!maybe_stream_ptr)
	{
		return maybe_stream_ptr.template ConvertFailureTo<Stream*>();
	}
	return MakeDriverSuccess<Stream*>(PushBackHandle(maybe_stream_ptr.TakeValue(), streams));
}

DriverResult<Reader*> RegisterReader(std::string&& bucket, std::string&& object)
{
	return RegisterStream<Reader>(MakeReaderPtr, std::move(bucket), std::move(object), active_reader_handles);
}

DriverResult<Writer*> RegisterWriter(std::string&& bucket, std::string&& object)
{
	return RegisterStream<Writer>(MakeWriterPtr, std::move(bucket), std::move(object), active_writer_handles);
}

void* driver_fopen(const char* filename, char mode)
{
	KH_AZ_CONNECTION_ERROR(nullptr);

	ERROR_ON_NULL_ARG(filename, nullptr);

	spdlog::debug("fopen {} {}", filename, mode);

	auto name_parsing_result = ParseAzureUri(filename);
	ERROR_ON_NAMES(name_parsing_result, nullptr);

	auto& parsed_names = name_parsing_result.GetValue();

	switch (mode)
	{
	case 'r':
	{
		auto register_res = RegisterReader(std::move(parsed_names.bucket), std::move(parsed_names.object));
		if (!register_res)
		{
			LogBadResult(register_res, "Error while opening reader stream.");
			return nullptr;
		}
		return register_res.GetValue();
	}
	    case 'w':
    {
        auto register_res = RegisterWriter(std::move(parsed_names.bucket), std::move(parsed_names.object));
        if (!register_res)
        {
            LogBadResult(register_res, "Error while opening writer stream.");
			return nullptr;
        }
        return register_res.GetValue();
    }
    /*
    case 'a':
    {
        // GCS does not as yet provide a way to add data to existing files.
        // This will be the process to emulate an append:
        // - check existence of the target object
        // - open a temporary write object to upload the new data
        // - compose, as defined by GCS, the source with the new temporary object
        //
        // The actual composition will happen on closing of the append stream

        auto maybe_list = ListObjects(names.bucket, names.object);
        if (!maybe_list)
        {
            // If file doesn't exist, fallback to write mode
            maybe_handle = RegisterWriter(std::move(names.bucket), std::move(names.object));
            err_msg = "Error while opening writer stream";
            break;
        }

        // go to end of list to get the target file name
        auto list_it = maybe_list->begin();
        const auto list_end = maybe_list->end();
        auto to_last_item = list_it;
        list_it++;
        while (list_end != list_it)
        {
            to_last_item = list_it;
            list_it++;
        }

        if (!to_last_item->ok())
        {
            // data is unusable
            maybe_handle = std::move(*to_last_item).status();
            err_msg = "Error opening file in append mode";
            break;
        }

        // get a writer handle
        maybe_handle = RegisterWriterForAppend(std::move(names.bucket), "tmp_object_to_append", to_last_item->value().name());
        err_msg = "Error opening file in append mode, cannot open tmp object";
        break;
    }
*/
	default:
		LogError("Invalid open mode: " + mode);
		return nullptr;
	}

	// RETURN_ON_ERROR(maybe_handle, err_msg, nullptr);

	// return *maybe_handle;

	// return NULL;
}

// #define ERROR_NO_STREAM(handle_it, errval)                                                                             \
// 	if ((handle_it) == active_handles.end())                                                                       \
// 	{                                                                                                              \
// 		LogError("Cannot identify stream");                                                                    \
// 		return (errval);                                                                                       \
// 	}

int driver_fclose(void* stream)
{
	assert(driver_isConnected());

	ERROR_ON_NULL_ARG(stream, kCloseEOF);

	spdlog::debug("fclose {}", (void*)stream);

	const auto reader_stream_it = FindReader(stream);
	if (reader_stream_it != active_reader_handles.end())
	{
		EraseRemove<Reader>(reader_stream_it, active_reader_handles);
		return kCloseSuccess;
	}

    const auto writer_stream_it = FindWriter(stream);
    if (writer_stream_it != active_writer_handles.end())
    {
        // finalize the block blob upload
        const auto& writer = **writer_stream_it;
		int res = kCloseEOF;
        try
        {
            writer.client_.CommitBlockList(writer.block_ids_list_);
            // the handle can be erased from its list, no staged block will remain
            res = kCloseSuccess;
        }
        catch(const StorageException& e)
        {
            // the staged blocks are still on the server but will get garbage collected after 10 days,
			// according to Azure service			
            LogException("Error while closing writer stream. Stream writer handle will be removed anyway.", e.what());
        }
		EraseRemove<Writer>(writer_stream_it, active_writer_handles);
		return res;
    }
	LogError("Cannot identify stream.");
	return kCloseEOF;
}

int driver_fseek(void* stream, long long int offset, int whence)
{
	KH_AZ_CONNECTION_ERROR(kBadSize);

	ERROR_ON_NULL_ARG(stream, kBadSize);

	spdlog::debug("fseek {} {} {}", stream, offset, whence);

	// confirm stream's presence
	auto reader_ptr_it = FindReader(stream);
	if (reader_ptr_it == active_reader_handles.end())
	{
		LogError("Cannot identify stream as a reader stream.");
		return kBadSize;
	}

	Reader& reader = **reader_ptr_it;

	constexpr long long max_val = std::numeric_limits<long long>::max();

	tOffset computed_offset{0};

	switch (whence)
	{
	case std::ios::beg:
		computed_offset = offset;
		break;
	case std::ios::cur:
		if (offset > max_val - reader.offset_)
		{
			LogError("Signed overflow prevented");
			return kBadSize;
		}
		computed_offset = reader.offset_ + offset;
		break;
	case std::ios::end:
		if (reader.total_size_ > 0)
		{
			long long minus1 = reader.total_size_ - 1;
			if (offset > max_val - minus1)
			{
				LogError("Signed overflow prevented");
				return kBadSize;
			}
		}
		if ((offset == std::numeric_limits<long long>::min()) && (reader.total_size_ == 0))
		{
			LogError("Signed overflow prevented");
			return kBadSize;
		}

		computed_offset = (reader.total_size_ == 0) ? offset : reader.total_size_ - 1 + offset;
		break;
	default:
		LogError("Invalid seek mode " + std::to_string(whence));
		return kBadSize;
	}

	if (computed_offset < 0)
	{
		LogError("Invalid seek offset " + std::to_string(computed_offset));
		return kBadSize;
	}
	reader.offset_ = computed_offset;
	return 0;
}

const char* driver_getlasterror()
{
	spdlog::debug("getlasterror");

	if (!lastError.empty())
	{
		return lastError.c_str();
	}
	return NULL;
}

long long int driver_fread(void* ptr, size_t size, size_t count, void* stream)
{
	KH_AZ_CONNECTION_ERROR(kBadSize);

	ERROR_ON_NULL_ARG(stream, kBadSize);
	ERROR_ON_NULL_ARG(ptr, kBadSize);

	if (0 == size)
	{
		LogError("Error passing size of 0");
		return kBadSize;
	}

	spdlog::debug("fread {} {} {} {}", ptr, size, count, stream);

	// confirm stream's presence
	const auto& reader_ptr_it = FindReader(stream);
	if (reader_ptr_it == active_reader_handles.end())
	{
		LogError("Cannot identify stream as a reader stream.");
		return kBadSize;
	}

	// fast exit for 0 read
	if (0 == count)
	{
		return 0;
	}

	// prevent overflow
	if (WillSizeCountProductOverflow(size, count))
	{
		LogError("product size * count is too large, would overflow");
		return kBadSize;
	}
	tOffset to_read{static_cast<tOffset>(size * count)};

	Reader& reader = **reader_ptr_it;
	const tOffset offset = reader.offset_;
	if (offset > std::numeric_limits<long long>::max() - to_read)
	{
		LogError("signed overflow prevented on reading attempt");
		return kBadSize;
	}
	// end of overflow prevention

	// special case: if offset >= total_size, error if not 0 byte required. 0 byte required is already done above
	const tOffset total_size = reader.total_size_;
	if (offset >= total_size)
	{
		LogError("Error trying to read more bytes while already out of bounds");
		return kBadSize;
	}

	// normal cases
	if (offset + to_read > total_size)
	{
		to_read = total_size - offset;
		spdlog::debug("offset {}, req len {} exceeds file size ({}) -> reducing len to {}", offset, to_read,
			      total_size, to_read);
	}
	else
	{
		spdlog::debug("offset = {} to_read = {}", offset, to_read);
	}

	const auto read_res = ReadBytesInFile(reader, reinterpret_cast<unsigned char*>(ptr), to_read);
	if (!read_res)
	{
		LogBadResult(read_res, "Error while reading from file.");
		return kBadSize;
	}

	return to_read;
}

long long int driver_fwrite(const void* ptr, size_t size, size_t count, void* stream)
{
	KH_AZ_CONNECTION_ERROR(kBadSize);

	ERROR_ON_NULL_ARG(stream, kBadSize);
	ERROR_ON_NULL_ARG(ptr, kBadSize);

	if (0 == size)
	{
		LogError("Error passing size 0 to fwrite");
		return kBadSize;
	}

	spdlog::debug("fwrite {} {} {} {}", ptr, size, count, stream);

	const auto& writer_ptr_it = FindWriter(stream);
	if (writer_ptr_it == active_writer_handles.end())
	{
		LogError("Cannot identify stream as a writer stream.");
		return kBadSize;
	}

	// fast exit for 0
	if (0 == count)
	{
		return 0;
	}

	// prevent integer overflow
	if (WillSizeCountProductOverflow(size, count))
	{
		LogError("Error on write: product size * count is too large, would overflow");
		return kBadSize;
	}

	// stage a block containing the data from the buffer
	auto& writer = **writer_ptr_it;
	const size_t to_write = size*count;
	Azure::Core::IO::MemoryBodyStream streambuf(reinterpret_cast<const uint8_t*>(ptr), to_write);
	
	const auto uuid = boost::uuids::random_generator()();
	std::vector<uint8_t> vec_uuid(uuid.size());
	std::copy(uuid.begin(), uuid.end(), vec_uuid.begin());
	std::string uuid_encoded = Azure::Core::Convert::Base64Encode(vec_uuid);
	
	try
	{
		const auto stage_response = writer.client_.StageBlock(uuid_encoded, streambuf);
		writer.block_ids_list_.push_back(std::move(uuid_encoded));
		return static_cast<long long>(to_write);
	}
	catch(const StorageException& e)
	{
		LogException("Error while writing data.", e.what());
	}
	catch(const std::exception& e)
	{
		LogException("Error while writing data.", e.what());
	}
	
	return kBadSize;
}

int driver_fflush(void* stream)
{
	KH_AZ_CONNECTION_ERROR(-1);

	ERROR_ON_NULL_ARG(stream, -1);

	const auto& writer_ptr_it = FindWriter(stream);
	if (writer_ptr_it == active_writer_handles.end())
	{
		LogError("Cannot identify stream as a writer stream.");
		return -1;
	}

	return 0;
}

int driver_remove(const char* filename)
{
	ERROR_ON_NULL_ARG(filename, kFailure);

	spdlog::debug("remove {}", filename);

	assert(driver_isConnected());

	auto maybe_names = GetServiceBucketAndObjectNames(filename);
	const auto& val = maybe_names.GetValue();

	std::string blobName = val.object;
	std::cout << "Deleting blob: " << blobName << std::endl;
	std::string containerName = val.bucket;
	auto containerClient = GetServiceClient<BlobServiceClient>().GetBlobContainerClient(containerName);

	// Create the block blob client
	BlockBlobClient blobClient = containerClient.GetBlockBlobClient(blobName);
	blobClient.Delete();

	/*
    auto maybe_names = GetBucketAndObjectNames(filename);
    ERROR_ON_NAMES(maybe_names, kFailure);
    auto& names = *maybe_names;

    const auto status = client.DeleteObject(names.bucket, names.object);
    if (!status.ok() && status.code() != gc::StatusCode::kNotFound)
    {
        LogBadStatus(status, "Error deleting object");
        return kFailure;
    }
*/
	return kSuccess;
}

int driver_rmdir(const char* filename)
{
	ERROR_ON_NULL_ARG(filename, kFailure);

	spdlog::debug("rmdir {}", filename);

	assert(driver_isConnected());
	spdlog::debug("Remove dir (does nothing...)");
	return kSuccess;
}

int driver_mkdir(const char* filename)
{
	ERROR_ON_NULL_ARG(filename, kFailure);

	spdlog::debug("mkdir {}", filename);

	assert(driver_isConnected());
	return kSuccess;
}

long long int driver_diskFreeSpace(const char* filename)
{
	ERROR_ON_NULL_ARG(filename, kFailure);

	spdlog::debug("diskFreeSpace {}", filename);

	assert(driver_isConnected());
	constexpr long long free_space{5LL * 1024LL * 1024LL * 1024LL * 1024LL};
	return free_space;
}

int driver_copyToLocal(const char* sSourceFilePathName, const char* sDestFilePathName)
{
	assert(driver_isConnected());

	if (!sSourceFilePathName || !sDestFilePathName)
	{
		LogError("Error passing null pointer to driver_copyToLocal");
		return kFailure;
	}

	spdlog::debug("copyToLocal {} {}", sSourceFilePathName, sDestFilePathName);
	/*
    auto maybe_names = GetBucketAndObjectNames(sSourceFilePathName);
    ERROR_ON_NAMES(maybe_names, kFailure);

    const std::string& bucket_name = maybe_names->bucket;
    const std::string& object_name = maybe_names->object;

    auto maybe_reader = MakeReaderPtr(bucket_name, object_name);
    RETURN_ON_ERROR(maybe_reader, "Error while opening Remote file", kFailure);

    ReaderPtr& reader = *maybe_reader;
    const size_t nb_files = reader->filenames_.size();

    // Open the local file
    std::ofstream file_stream(sDestFilePathName, std::ios::binary);
    if (!file_stream.is_open())
    {
        std::ostringstream os;
        os << "Failed to open local file for writing: " << sDestFilePathName;
        LogError(os.str());
        return kFailure;
    }

    // Allocate a relay buffer
    constexpr size_t buf_size{1024};
    std::array<char, buf_size> buffer{};
    char *buf_data = buffer.data();

    // create a waste buffer now, so the lambdas can reference it
    // memory allocation will occur later, before actual use
    std::vector<char> waste;

    auto read_and_write = [&](gcs::ObjectReadStream &from, bool skip_header = false, std::streamsize header_size = 0)
    {
        if (!from)
        {
            LogBadStatus(from.status(), "Error initializing download stream");
            return false;
        }

        if (skip_header)
        {
            // according to gcs sources, seekg is not implemented
            // waste a read on the first bytes
            if (!from.read(waste.data(), header_size))
            {
                // check failure reasons to give feedback
                std::string err_msg;
                if (from.eof())
                {
                    err_msg = "Error reading header. Shorter header than expected";
                }
                else if (from.bad())
                {
                    err_msg = "Error reading header. Read failed";
                }
                LogBadStatus(from.status(), err_msg);
                return false;
            }
        }

        const std::streamsize buf_size_cast = static_cast<std::streamsize>(buf_size);
        while (from.read(buf_data, buf_size_cast) && file_stream.write(buf_data, buf_size_cast))
        {
        }
        // what made the process stop?
        if (!file_stream)
        {
            // something went wrong on write side, abort
            LogError("Error while writing data to local file");
            return false;
        }
        else if (from.eof())
        {
            // short read, copy what remains, if any
            const std::streamsize rem = from.gcount();
            if (rem > 0 && !file_stream.write(buf_data, rem))
            {
                // something went wrong on write side, abort
                LogError("Error while writing data to local file");
                return false;
            }
        }
        else if (from.bad())
        {
            // something went wrong on read side
            LogBadStatus(from.status(), "Error while reading from cloud storage");
            return false;
        }

        return true;
    };

    auto operation = [&](gcs::ObjectReadStream &from, const std::string &filename, bool skip_header = false, tOffset header_size = 0)
    {
        from = client.ReadObject(bucket_name, filename);
        bool res = read_and_write(from, skip_header, header_size);
        from.Close();
        return res;
    };

    auto &filenames = reader->filenames_;

    // Read the whole first file
    gcs::ObjectReadStream read_stream;
    if (!operation(read_stream, filenames.front()))
    {
        return kFailure;
    }

    // fast exit
    if (nb_files == 1)
    {
        return kSuccess;
    }

    // Read from the next files
    const tOffset header_size = reader->commonHeaderLength_;
    const bool skip_header = header_size > 0;
    waste.reserve(static_cast<size_t>(header_size));

    for (size_t i = 1; i < nb_files; i++)
    {
        if (!operation(read_stream, filenames[i], skip_header, header_size))
        {
            return kFailure;
        }
    }

    // done copying
    spdlog::debug("Done copying");
*/

	return kSuccess;
}

int driver_copyFromLocal(const char* sSourceFilePathName, const char* sDestFilePathName)
{
	if (!sSourceFilePathName || !sDestFilePathName)
	{
		LogError("Error passing null pointers as arguments to copyFromLocal");
		return kFailure;
	}

	spdlog::debug("copyFromLocal {} {}", sSourceFilePathName, sDestFilePathName);

	assert(driver_isConnected());
	/*
    auto maybe_names = GetBucketAndObjectNames(sDestFilePathName);
    ERROR_ON_NAMES(maybe_names, kFailure);

    // Open the local file
    std::ifstream file_stream(sSourceFilePathName, std::ios::binary);
    if (!file_stream.is_open())
    {
        std::ostringstream os;
        os << "Failed to open local file: " << sSourceFilePathName;
        LogError(os.str());
        return kFailure;
    }

    // Create a WriteObject stream
    const auto& names = *maybe_names;
    auto writer = client.WriteObject(names.bucket, names.object);
    if (!writer || !writer.IsOpen())
    {
        LogBadStatus(writer.metadata().status(), "Error initializing upload stream to remote storage");
        return kFailure;
    }

    // Read from the local file and write to the GCS object
    constexpr size_t buf_size{1024};
    std::array<char, buf_size> buffer{};
    char *buf_data = buffer.data();

    while (file_stream.read(buf_data, buf_size) && writer.write(buf_data, buf_size)){}
    // what made the process stop?
    if (!writer)
    {
        LogBadStatus(writer.last_status(), "Error while copying to remote storage");
        return kFailure;
    }
    else if (file_stream.eof())
    {
        // copy what remains in the buffer
        const auto rem = file_stream.gcount();
        if (rem > 0 && !writer.write(buf_data, rem))
        {
            LogError("Error while copying to remote storage");
            return kFailure;
        }
    }
    else if (file_stream.bad())
    {
        LogError("Error while reading on local storage");
        return kFailure;
    }

    // Close the GCS WriteObject stream to complete the upload
    writer.Close();

    auto &maybe_meta = writer.metadata();
    RETURN_ON_ERROR(maybe_meta, "Error during file upload to remote storage", kFailure);
*/
	return kSuccess;
}
