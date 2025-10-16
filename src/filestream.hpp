// Input and output file streams, for reading from / writing to blobs and share
// files.

#pragma once

#include "exception.hpp"
#include "filestream.hpp"
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

  static FileStream
  OpenForReading(const std::vector<Azure::Storage::Blobs::BlobClient> &clients);
  static FileStream OpenForReading(
      const std::vector<Azure::Storage::Files::Shares::ShareFileClient>
          &clients);
  static FileStream OpenForReading(const std::vector<ObjectClient> &clients);
  static FileStream
  OpenForWriting(OutputMode mode,
                 const Azure::Storage::Blobs::BlobClient &client);
  static FileStream
  OpenForWriting(OutputMode mode,
                 const Azure::Storage::Files::Shares::ShareFileClient &client);
  static FileStream OpenForWriting(OutputMode mode, const ObjectClient &client);
  FileStream(FileStream &&source);
  ~FileStream();

  void *GetHandle() const;

  void Close();

  // Reader-only operations
  size_t Read(void *dest, size_t nSize, size_t nCount);
  void Seek(long long int nOffset, int nOrigin);

  // Writer-only operations
  size_t Write(const void *source, size_t nSize, size_t nCount);
  void Flush();

private:
  FileStream();

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

class InvalidOperationForStreamModeError : public Error {
public:
  inline InvalidOperationForStreamModeError(const std::string &operation,
                                            FileStream::Mode mode)
      : Error((std::ostringstream()
               << "operation '" << operation << "' is invalid for stream mode '"
               << (mode == FileStream::Mode::READ ? "reader" : "writer") << "'")
                  .str()) {}
};
} // namespace az
