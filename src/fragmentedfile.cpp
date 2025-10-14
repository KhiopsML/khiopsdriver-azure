#include "fragmentedfile.hpp"
#include <string>
#include <algorithm>
#include <numeric>
#include <memory>
#include <azure/core/http/http.hpp>
#include <azure/storage/blobs/rest_client.hpp>
#include <azure/storage/files/shares/share_responses.hpp>
#include "util.hpp"

using namespace std;
using HttpRange = Azure::Core::Http::HttpRange;
using BodyStream = Azure::Core::IO::BodyStream;
using DownloadBlobOptions = Azure::Storage::Blobs::DownloadBlobOptions;
using DownloadFileOptions = Azure::Storage::Files::Shares::DownloadFileOptions;

namespace az
{
	static string ReadHeaderFromBodyStream(std::unique_ptr<BodyStream>&& bodyStream);

	static constexpr size_t nMaxHeaderLen = 8ULL * 1024 * 1024;

	FragmentedFile::Fragment::Fragment(size_t nContentSize, const ObjectClient& client, const Azure::ETag& etag) :
		nUserOffset(0ULL),
		nContentSize(nContentSize),
		client(client),
		etag(etag)
	{}

	FragmentedFile::Fragment::Fragment(Fragment&& source) :
		nUserOffset(std::move(source.nUserOffset)),
		nContentSize(std::move(source.nContentSize)),
		client(source.client),
		etag(std::move(source.etag))
	{}

	FragmentedFile::Fragment& FragmentedFile::Fragment::operator=(Fragment&& source)
	{
		nUserOffset = std::move(source.nUserOffset);
		nContentSize = std::move(source.nContentSize);
		client = source.client;
		etag = std::move(source.etag);
		return *this;
	}

	FragmentedFile::FragmentedFile() :
		nHeaderLen(0),
		nSize(0)
	{}

	FragmentedFile::FragmentedFile(const vector<Azure::Storage::Blobs::BlobClient>& clients) :
		FragmentedFile(vector<ObjectClient>(clients.begin(), clients.end()))
	{}

	FragmentedFile::FragmentedFile(const vector<Azure::Storage::Files::Shares::ShareFileClient>& clients) :
		FragmentedFile(vector<ObjectClient>(clients.begin(), clients.end()))
	{}

	FragmentedFile::FragmentedFile(const vector<ObjectClient>& clients):
		storageType(clients.front().tag),
		nHeaderLen(0ULL),
		nSize(0ULL)
	{
		if (clients.empty()) return;

		HttpRange range { 0, nMaxHeaderLen };
		size_t nClients = clients.size();
		size_t nRandomlyPicked = 0;
		string sHeader;
		string sFragmentHeader;
		size_t nFragmentSize;
		Azure::ETag etag;
		bool bGetHeader;
		bool bMaybeHeader = true;

		for (size_t i = 0; i < nClients; i++)
		{
			// Determine if we need to fetch the header of the current fragment
			if (!bMaybeHeader) bGetHeader = false;
			else if (i < 5 || nClients <= 10 || i >= nClients - 5) bGetHeader = true;
			else if (nRandomlyPicked < 10 && (i >= nClients - 15 + nRandomlyPicked || util::random::RandomBool()))
			{
				bGetHeader = true;
				nRandomlyPicked++;
			}
			else bGetHeader = false;

			if (clients[i].tag == BLOB)
			{
				auto blobProperties = std::move(clients[i].blob.GetProperties().Value);
				nFragmentSize = (size_t)blobProperties.BlobSize;
				etag = blobProperties.ETag;
				if (bGetHeader && nFragmentSize >= nHeaderLen)
				{
					DownloadBlobOptions opts;
					opts.Range = range;
					sFragmentHeader = ReadHeaderFromBodyStream(std::move(clients[i].blob.Download(opts).Value.BodyStream));
				}
			}
			else // SHARE storage
			{
				auto fileProperties = std::move(clients[i].shareFile.GetProperties().Value);
				nFragmentSize = (size_t)fileProperties.FileSize;
				etag = fileProperties.ETag;
				if (bGetHeader && nFragmentSize >= nHeaderLen)
				{
					DownloadFileOptions opts;
					opts.Range = range;
					sFragmentHeader = ReadHeaderFromBodyStream(std::move(clients[i].shareFile.Download(opts).Value.BodyStream));
				}
			}

			if (i == 0)
			{
				sHeader = sFragmentHeader;
				nHeaderLen = sHeader.length();
				range.Length = nHeaderLen;
				if (nHeaderLen == 0) bMaybeHeader = false;
			}

			if (bMaybeHeader && bGetHeader && sFragmentHeader != sHeader)
			{
				bMaybeHeader = false;
				nHeaderLen = 0;
			}

			fragments.emplace_back(nFragmentSize, clients[i], etag);
		}

		size_t nFreePosition = 0ULL;
		for (size_t i = 0ULL; i < fragments.size(); i++)
		{
			if (i != 0ULL) fragments[i].nContentSize -= nHeaderLen;
			if (fragments[i].nContentSize != 0ULL)
			{
				fragments[i].nUserOffset = nSize;
				nSize += fragments[i].nContentSize;
				if (i != nFreePosition) fragments[nFreePosition] = std::move(fragments[i]);
				nFreePosition++;
			}
		}
		fragments.erase(fragments.begin() + nFreePosition, fragments.end());
	}

	FragmentedFile::FragmentedFile(FragmentedFile&& source) :
		storageType(std::move(source.storageType)),
		nHeaderLen(std::move(source.nHeaderLen)),
		nSize(std::move(source.nSize)),
		fragments(std::move(source.fragments))
	{}

	FragmentedFile::~FragmentedFile()
	{
		fragments.clear();
	}

	size_t FragmentedFile::GetSize() const
	{
		return nSize;
	}

	size_t FragmentedFile::GetHeaderLen() const
	{
		return nHeaderLen;
	}

	const FragmentedFile::Fragment& FragmentedFile::GetFragment(size_t nIndex) const
	{
		return fragments.at(nIndex);
	}

	size_t FragmentedFile::GetFragmentIndexOfUserOffset(size_t nUserOffset) const
	{
		if (fragments.empty())
		{
			throw NoFragmentError();
		}
		return (size_t)(find_if(fragments.begin(), fragments.end(), [nUserOffset](const auto& fragment) { return nUserOffset < fragment.nUserOffset; }) - 1 - fragments.begin());
	}

	static string ReadHeaderFromBodyStream(unique_ptr<BodyStream>&& bodyStream)
	{
		string sHeader;
		size_t nBufferSize = (size_t)bodyStream->Length();
		uint8_t* bufferEnd;
		uint8_t* foundLineFeed;
		bool bFoundLineFeed;

		uint8_t* buffer = new uint8_t[nBufferSize];
		try
		{
			bodyStream->ReadToCount(buffer, nBufferSize);
			bufferEnd = buffer + nBufferSize;
			bFoundLineFeed = (foundLineFeed = find(buffer, bufferEnd, '\n')) < bufferEnd;
			sHeader.append((const char*)buffer, bFoundLineFeed ? foundLineFeed + 1 - buffer : nBufferSize);
		}
		catch (...)
		{
			delete[] buffer;
			throw;
		}
		delete[] buffer;

		return bFoundLineFeed ? sHeader : "";
	}
}
