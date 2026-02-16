// Input and output file streams, for reading from / writing to blobs and share
// files.

#pragma once

#include "fragmentedfile.hpp"
#include "objectclient.hpp"
#include "storagetype.hpp"
#include <azure/storage/blobs/blob_client.hpp>
#include <azure/storage/files/shares/share_file_client.hpp>
#include <cstddef>
#include <sstream>
#include <string>
#include <vector>

namespace az {
class FileStream {
public:
  enum class Mode { READ, WRITE };
  enum class OutputMode { WRITE, APPEND };

  static int
  OpenForReading(FileStream *result,
                 const std::vector<Azure::Storage::Blobs::BlobClient> &clients);
  static int OpenForReading(
      FileStream *result,
      const std::vector<Azure::Storage::Files::Shares::ShareFileClient>
          &clients);
  static int OpenForReading(FileStream *result,
                            const std::vector<ObjectClient> &clients);
  static void OpenForWriting(FileStream *result, OutputMode mode,
                             const Azure::Storage::Blobs::BlobClient &client);
  static void
  OpenForWriting(FileStream *result, OutputMode mode,
                 const Azure::Storage::Files::Shares::ShareFileClient &client);
  static void OpenForWriting(FileStream *result, OutputMode mode,
                             const ObjectClient &client);
  FileStream();
  FileStream(FileStream &&source);
  FileStream &operator=(FileStream &&other);
  ~FileStream();

  void *GetHandle() const;
  Mode GetMode() const;

  void Close();

  // Reader-only operations
  int Read(size_t *nRead, void *dest, size_t nSize, size_t nCount);
  int Seek(long long int nOffset, int nOrigin);

  // Writer-only operations
  int Write(size_t *nWritten, const void *source, size_t nSize, size_t nCount);
  int Flush();

private:
  void *handle;

  StorageType storageType;
  Mode mode;
  size_t nCurrentPos;

  struct WriteInfo {
    OutputMode mode;
    ObjectClient client;
    std::vector<std::string> blockIds;
    WriteInfo(OutputMode mode, const ObjectClient &client,
              const std::vector<std::string> &blockIds);
    WriteInfo(WriteInfo &&source);
    ~WriteInfo();
  };

  union {
    FragmentedFile readInfo; // Reader-only attributes
    WriteInfo writeInfo;     // Writer-only attributes
  };
};
} // namespace az
