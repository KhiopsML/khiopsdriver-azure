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
	static struct FragmentForComputation
	{
		string sHeader;
		size_t nSize;
		ObjectClient client;

		FragmentForComputation(string&& sHeader, size_t nSize, const ObjectClient& client);
		FragmentForComputation(const FragmentForComputation& other);
		FragmentForComputation& operator=(FragmentForComputation&& source);
	};

	static string ReadHeaderFromBodyStream(std::unique_ptr<BodyStream>&& bodyStream);
	static string GetFileHeader(const vector<FragmentForComputation>& fragments, vector<size_t>&& fragmentsWithHeadersToInspect);
	static vector<FragmentedFile::Fragment> GetFileFragments(vector<FragmentForComputation>& fragments, size_t nHeaderLen);

	static constexpr size_t nMaxHeaderSize = 8ULL * 1024 * 1024;

	FragmentForComputation::FragmentForComputation(string&& sHeader, size_t nSize, const ObjectClient& client) :
		sHeader(sHeader),
		nSize(nSize),
		client(client)
	{}

	FragmentForComputation::FragmentForComputation(const FragmentForComputation& other) :
		sHeader(other.sHeader),
		nSize(other.nSize),
		client(other.client)
	{}

	FragmentForComputation& FragmentForComputation::operator=(FragmentForComputation&& source)
	{
		sHeader = move(source.sHeader);
		nSize = move(source.nSize);
		client = move(source.client);
		return *this;
	}

	FragmentedFile::Fragment::Fragment(size_t nUserOffset, size_t nContentSize, const ObjectClient& client) :
		nUserOffset(nUserOffset),
		nContentSize(nContentSize),
		client(client)
	{}

	FragmentedFile::Fragment::Fragment(Fragment&& source) :
		nUserOffset(move(source.nUserOffset)),
		nContentSize(move(source.nContentSize)),
		client(source.client)
	{}

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
		storageType(clients.front().tag)
	{
		if (clients.empty())
		{
			nHeaderLen = 0;
			nSize = 0;
			return;
		}

		vector<FragmentForComputation> fragmentsForComputation;
		HttpRange range { 0, nMaxHeaderSize };
		size_t nClients = clients.size();
		size_t nRandomlyPicked = 0;
		string sFragmentHeader;
		size_t nFragmentSize;
		vector<size_t> fragmentsWithHeadersToInspect;
		size_t nFirstFragmentHeaderLen;

		for (size_t i = 0; i < nClients; i++)
		{
			sFragmentHeader = "";
			bool bGetHeader = false;
			if (i < 5 || nClients <= 10 || i >= nClients - 5) bGetHeader = true;
			else if (nRandomlyPicked < 10 && (i >= nClients - 15 + nRandomlyPicked || util::random::RandomBool()))
			{
				bGetHeader = true;
				nRandomlyPicked++;
			}
			if (clients[i].tag == BLOB)
			{
				if (bGetHeader)
				{
					DownloadBlobOptions opts;
					opts.Range = range;
					auto downloadResult = move(clients[i].blob.Download(opts).Value);
					sFragmentHeader = ReadHeaderFromBodyStream(move(downloadResult.BodyStream));
					if (i == 0) nFirstFragmentHeaderLen = sFragmentHeader.length();
					else opts.Range->Length = nFirstFragmentHeaderLen;
					nFragmentSize = (size_t)downloadResult.BlobSize;
				}
				else nFragmentSize = (size_t)clients[i].blob.GetProperties().Value.BlobSize;
			}
			else // SHARE storage
			{
				if (bGetHeader)
				{
					DownloadFileOptions opts;
					opts.Range = range;
					auto downloadResult = move(clients[i].shareFile.Download(opts).Value);
					sFragmentHeader = ReadHeaderFromBodyStream(move(downloadResult.BodyStream));
					if (i == 0) nFirstFragmentHeaderLen = sFragmentHeader.length();
					else opts.Range->Length = nFirstFragmentHeaderLen;
					opts.Range->Length = nFirstFragmentHeaderLen;
					nFragmentSize = (size_t)downloadResult.FileSize;
				}
				else nFragmentSize = (size_t)clients[i].shareFile.GetProperties().Value.FileSize;
			}
			fragmentsForComputation.emplace_back(move(sFragmentHeader), nFragmentSize, clients[i]);
			if (bGetHeader) fragmentsWithHeadersToInspect.push_back(i);
		}

		nHeaderLen = GetFileHeader(fragmentsForComputation, move(fragmentsWithHeadersToInspect)).size();
		fragments = GetFileFragments(fragmentsForComputation, nHeaderLen);
		nSize = accumulate(fragments.begin(), fragments.end(), 0ULL, [](size_t nTotal, const Fragment& fragment) { return nTotal + fragment.nContentSize; });
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
		constexpr size_t nBufferSize = 4096; // TODO
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

	static string GetFileHeader(const vector<FragmentForComputation>& fragments, vector<size_t>&& fragmentsWithHeadersToInspect)
	{
		string sFirstHeader = fragments.at(fragmentsWithHeadersToInspect.front()).sHeader;
		return sFirstHeader.empty()
			|| any_of(fragmentsWithHeadersToInspect.begin() + 1, fragmentsWithHeadersToInspect.end(), [sFirstHeader, &fragments](auto fragmentIndex) { return fragments[fragmentIndex].sHeader != sFirstHeader; })
			? string()
			: sFirstHeader;
	}

	static vector<FragmentedFile::Fragment> GetFileFragments(vector<FragmentForComputation>& fragments, size_t nHeaderLen)
	{
		vector<FragmentedFile::Fragment> result;
		size_t nUserOffset = 0;
		size_t nContentSize;
		bool bFirstIter = true;
		for (auto& fragment : fragments)
		{
			nContentSize = bFirstIter ? fragment.nSize : fragment.nSize - nHeaderLen;
			if (nContentSize != 0)
			{
				result.emplace_back(nUserOffset, nContentSize, move(fragment.client));
			}
			nUserOffset += nContentSize;
			bFirstIter = false;
		}
		return result;
	}
}
