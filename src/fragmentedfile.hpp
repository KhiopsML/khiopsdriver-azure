/*
Remote file abstraction that understands a file might have been split into
multiple remote files. This is also used for monolythic files, which are
considered as single-fragment files.
Multi-fragment remote files are specified in URLs using globbing.
Empty fragments, including fragments containing only the header (if any),
are removed. Thus, the number of resulting fragments may be less than the
number of remote files.
*/

#pragma once

#include "objectclient.hpp"
#include "storagetype.hpp"
#include <azure/core/io/body_stream.hpp>
#include <azure/storage/blobs/blob_client.hpp>
#include <azure/storage/files/shares/share_file_client.hpp>
#include <cstddef>
#include <vector>

namespace az {
class FragmentedFile {
public:
  struct Fragment {
    // User offset is the start position of this fragment in the whole
    // fragmented file as seen by the user. If the fragmented file
    // contains a header, the user only sees the header at the beginning
    // of the first fragment. User offset is always zero for the first
    // fragment.
    size_t nUserOffset;
    // Content size includes the header length only for the first fragment.
    size_t nContentSize;
    ObjectClient client;
    Azure::ETag etag;

    Fragment(size_t nContentSize, const ObjectClient &client,
             const Azure::ETag &etag);
    Fragment(Fragment &&source);
    Fragment &operator=(Fragment &&source);
  };

  FragmentedFile();
  FragmentedFile(const std::vector<Azure::Storage::Blobs::BlobClient> &clients);
  FragmentedFile(
      const std::vector<Azure::Storage::Files::Shares::ShareFileClient>
          &clients);
  FragmentedFile(const std::vector<ObjectClient> &clients);
  FragmentedFile(FragmentedFile &&source);
  ~FragmentedFile();
  size_t GetSize() const;
  size_t GetHeaderLen() const;
  const Fragment &GetFragment(size_t nIndex) const;
  int GetFragmentIndexOfUserOffset(size_t *nFragmentIndex,
                                   size_t nUserOffset) const;
  size_t GetNumberOfFragments() const;

private:
  StorageType storageType;
  size_t nHeaderLen;
  size_t nSize;
  std::vector<Fragment> fragments;
};
} // namespace az
