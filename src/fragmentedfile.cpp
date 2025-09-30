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
		nUserOffset(move(source.nUserOffset)),
		nContentSize(move(source.nContentSize)),
		client(source.client),
		etag(move(source.etag))
	{}

	FragmentedFile::Fragment& FragmentedFile::Fragment::operator=(Fragment&& source)
	{
		nUserOffset = move(source.nUserOffset);
		nContentSize = move(source.nContentSize);
		client = source.client;
		etag = move(source.etag);
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
				nFragmentSize = (size_t)clients[i].blob.GetProperties().Value.BlobSize;
				if (bGetHeader && nFragmentSize >= nHeaderLen)
				{
					DownloadBlobOptions opts;
					opts.Range = range;
					auto downloadResult = move(clients[i].blob.Download(opts).Value);
					etag = downloadResult.Details.ETag;
					sFragmentHeader = ReadHeaderFromBodyStream(move(downloadResult.BodyStream));
				}
			}
			else // SHARE storage
			{
				nFragmentSize = (size_t)clients[i].shareFile.GetProperties().Value.FileSize;
				if (bGetHeader && nFragmentSize >= nHeaderLen)
				{
					DownloadFileOptions opts;
					opts.Range = range;
					auto downloadResult = move(clients[i].shareFile.Download(opts).Value);
					etag = downloadResult.Details.ETag;
					sFragmentHeader = ReadHeaderFromBodyStream(move(downloadResult.BodyStream));
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
				if (i != nFreePosition) fragments[nFreePosition] = move(fragments[i]);
				nFreePosition++;
			}
		}
	}

	FragmentedFile::FragmentedFile(FragmentedFile&& source) :
		storageType(move(source.storageType)),
		nHeaderLen(move(source.nHeaderLen)),
		nSize(move(source.nSize)),
		fragments(move(source.fragments))
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
		constexpr size_t nBufferSize = 4096; // TODO: can be adjusted if needed
		size_t nBytesRead;
		uint8_t* bufferReadEnd;
		uint8_t* foundLineFeed;
		bool bFoundLineFeed;

		uint8_t* buffer = new uint8_t[nBufferSize];
		try
		{
			do
			{
				nBytesRead = bodyStream->ReadToCount(buffer, nBufferSize);
				bufferReadEnd = buffer + nBytesRead;
				bFoundLineFeed = (foundLineFeed = find(buffer, bufferReadEnd, '\n')) < bufferReadEnd;
				sHeader.append((const char*)buffer, bFoundLineFeed ? foundLineFeed + 1 - buffer : nBytesRead);
			} while (!bFoundLineFeed && nBytesRead == nBufferSize);
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
