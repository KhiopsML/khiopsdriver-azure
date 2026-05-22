#include "khiops_driver_common/backend.hpp"
#include "khiops_driver_common/logging.hpp"
#include "khiops_driver_common/util.hpp"
#include "khiops_driver_azure/util.hpp"
#include "khiops_driver_azure/version.hpp"
#include "khiops_driver_azure/servicerequest.hpp"
#include "khiops_driver_azure/fragmentedfile.hpp"
#include <memory>
#include <spdlog/spdlog.h>
#include <azure/core/diagnostics/logger.hpp>

using namespace std;
using namespace khiops_driver_azure;

namespace khiops_driver_common {

spdlog::logger *GetLogger() {
    return GetLogger("azdriver", "AZURE_DRIVER_LOGFILE", "AZURE_DRIVER_LOGLEVEL");
}

int GetDriverName(std::string *result) {
    *result = "Azure driver";
    return 0;
}

int GetDriverVersion(std::string *result) {
    *result = DRIVER_VERSION;
    return 0;
}

int GetDriverScheme(std::string *result) {
    *result = "https";
    return 0;
}

int IsReadOnly(bool *result) {
    *result = false;
    return 0;
}

int Initialize() {
    // Disable Azure SDK logging.
    // Note: This will not prevent Azure CLI, called as a subprocess by the
    // Azure SDK, to log errors such as "Please run 'az login' to authenticate".
    Azure::Core::Diagnostics::Logger::SetListener([](Azure::Core::Diagnostics::Logger::Level, string const &) {});
    return 0;
}

int Finalize() {
    // Nothing to do.
    return 0;
}

int GetSystemPreferredBufferSize(size_t *result) {
    constexpr size_t DEFAULT_PREFERRED_BUFFER_SIZE = 4ULL * 1024ULL * 1024ULL;
    const string ENVIRONMENT_VARIABLE_NAME = "AZURE_PREFERRED_BUFFER_SIZE";
    static unique_ptr<size_t> system_preferred_buffer_size_memo = nullptr;
    if (system_preferred_buffer_size_memo == nullptr) {
        string environment_variable_preferred_buffer_size = util::env::GetEnvVar(ENVIRONMENT_VARIABLE_NAME);
        if (!environment_variable_preferred_buffer_size.empty()) {
            try {
                *result = stoull(environment_variable_preferred_buffer_size);
                system_preferred_buffer_size_memo = make_unique<size_t>(*result);
                return 0;
            } catch (const invalid_argument &) {
                GetLogger()->warn(
                    "Value {} of environment variable {} is not a valid number. Falling back to default {}...",
                    environment_variable_preferred_buffer_size, ENVIRONMENT_VARIABLE_NAME, DEFAULT_PREFERRED_BUFFER_SIZE
                );
            } catch (const out_of_range &) {
                GetLogger()->warn(
                    "Value {} of environment variable {} is out of range. Falling back to default {}...",
                    environment_variable_preferred_buffer_size, ENVIRONMENT_VARIABLE_NAME, DEFAULT_PREFERRED_BUFFER_SIZE
                );
            }
        }
        *result = DEFAULT_PREFERRED_BUFFER_SIZE;
        system_preferred_buffer_size_memo = make_unique<size_t>(*result);
    } else {
        *result = *system_preferred_buffer_size_memo;
    }
    return 0;
}

int FileExists(bool *result, const std::string &sFilePathName) {
    ServiceRequest request;
    if (ParseUrl(&request, sFilePathName)) {
        return -1;
    }
    if (request.storageType == BLOB) {
        *result = !ListBlobs(request).empty();
    } else /* SHARE */ {
        *result = !ListFiles(request).empty();
    }
    return 0;
}

int DirExists(bool *result, const std::string &sFilePathName) {
    ServiceRequest request;
    if (ParseUrl(&request, sFilePathName)) {
        return -1;
    }
    if (request.storageType == BLOB) {
        *result = true;  // there is no such concept as a directory when dealing with blob services
    } else /* SHARE */ {
        *result = !ListDirs(request).empty();
    }
    return 0;
}

int GetFileSize(size_t *result, const std::string &filename) {
    ServiceRequest request;
    if (ParseUrl(&request, filename)) {
        return -1;
    }
    if (request.storageType == BLOB) {
        auto blobs = ListBlobs(request);
        if (blobs.empty()) {
            GetLogger()->error("No blob matches URL {}.", filename);
            return -1;
        }
        *result = FragmentedFile(std::move(blobs)).GetSize();
    } else /* SHARE */ {
        auto files = ListFiles(request);
        if (files.empty()) {
            GetLogger()->error("No file matches URL {}.", filename);
            return -1;
        }
        *result = FragmentedFile(std::move(files)).GetSize();
    }
    return 0;
}

int FOpen(khiops_driver_common::FileStream &stream, const std::string &filename) {
    GetLogger()->error("Not implemented!");
    return -1;
}

int FClose(const khiops_driver_common::FileStream &stream) {
    GetLogger()->error("Not implemented!");
    return -1;
}

int FRead(size_t *result, void *ptr, size_t size, size_t count, khiops_driver_common::FileStream &stream) {
    GetLogger()->error("Not implemented!");
    return -1;
}

int FSeek(khiops_driver_common::FileStream &stream, long long int offset, int whence) {
    GetLogger()->error("Not implemented!");
    return -1;
}

int FWrite(size_t *result, const void *ptr, size_t size, size_t count, const khiops_driver_common::FileStream &stream) {
    GetLogger()->error("Not implemented!");
    return -1;
}

int FFlush(const khiops_driver_common::FileStream &stream) {
    GetLogger()->error("Not implemented!");
    return -1;
}

int Remove(const std::string &filename) {
    ServiceRequest request;
    if (ParseUrl(&request, filename)) {
        return -1;
    }
    if (request.storageType == BLOB) {
        auto blobs = ListBlobs(request);
        if (blobs.empty()) {
            GetLogger()->error("No blob matches URL {}.", filename);
            return -1;
        }
        Azure::Storage::Blobs::DeleteBlobOptions opts;
        opts.DeleteSnapshots =
                Azure::Storage::Blobs::Models::DeleteSnapshotsOption::IncludeSnapshots;
        for (const auto &blob : blobs) {
            const string sBlobUrl = blob.GetUrl();
            if (!blob.Delete(opts).Value.Deleted) {
                GetLogger()->error("Failed to delete blob {}.", sBlobUrl);
                return -1;
            }
        }
    } else /* SHARE */ {
        auto files = ListFiles(request);
        if (files.empty()) {
            GetLogger()->error("No file matches URL {}.", filename);
            return -1;
        }
        for (const auto &file : files) {
            const string sFileUrl = file.GetUrl();
            if (!file.Delete().Value.Deleted) {
                GetLogger()->error("Failed to delete file {}.", sFileUrl);
                return -1;
            }
        }
    }
    return 0;
}

int Mkdir(const std::string &pathname) {
    ServiceRequest request;
    if (ParseUrl(&request, pathname)) {
        return -1;
    }
    if (request.storageType == BLOB) {
        GetLogger()->info("Making a directory for a blob storage does nothing.");
    } else // SHARE
    {
        string sNewDir = request.share.path.back();
        Azure::Storage::Files::Shares::ShareDirectoryClient parentDir("");
        if (GetParentDir(&parentDir, request)) {
            return -1;
        }

        Azure::Storage::Files::Shares::ListFilesAndDirectoriesOptions opts;
        opts.Prefix = sNewDir;
        for (auto pagedResponse = parentDir.ListFilesAndDirectories(opts);
                 pagedResponse.HasPage(); pagedResponse.MoveToNextPage()) {
            if (find_if(pagedResponse.Directories.begin(),
                          pagedResponse.Directories.end(),
                          [sNewDir](const auto &dirItem) {
                            return dirItem.Name == sNewDir;
                          }) != pagedResponse.Directories.end()) {
                GetLogger()->error("Cannot make directory: directory already exists.");
                return -1;
            }
        }

        if (!parentDir.GetSubdirectoryClient(sNewDir).Create().Value.Created) {
            GetLogger()->error("Failed to make directory.");
            return -1;
        }
    }
    return 0;
}

int Rmdir(const std::string &pathname) {
    ServiceRequest request;
    if (ParseUrl(&request, pathname)) {
        return -1;
    }
    if (request.storageType == BLOB) {
        GetLogger()->info("Removing a directory with a blob storage does nothing.");
    } else // SHARE
    {
        auto dirs = ListDirs(request);
        if (dirs.empty()) {
            GetLogger()->error("No directory matches URL {}.", pathname);
            return -1;
        }
        for (const auto &dir : dirs) {
            const string sDirUrl = dir.GetUrl();
            if (!dir.Delete().Value.Deleted) {
                GetLogger()->error("Failed to delete directory {}.", sDirUrl);
                return -1;
            }
        }
    }
    return 0;
}

int DiskFreeSpace(size_t *result, const std::string &filename) {
    *result = 5ULL * 1024ULL * 1024ULL * 1024ULL * 1024ULL;
    return 0;
}

int CopyToLocal(const std::string &sourcefilename, const std::string &destfilename) {
    ServiceRequest request;
    if (ParseUrl(&request, sourcefilename)) {
        return -1;
    }
    FileStream *readerPtr;
    if (OpenForReading(&readerPtr, sourcefilename)) {
        return -1;
    }
    size_t system_preferred_buffer_size;
    if (GetSystemPreferredBufferSize(&system_preferred_buffer_size) != 0) {
        return -1;
    }
    std::unique_ptr<char[]> buffer = std::make_unique<char[]>(system_preferred_buffer_size);
    ofstream ofs(destfilename, ios::binary);
    size_t nRead;

    for (;;) {
        GetLogger()->trace("Copying at most {} bytes from remote to local file...", system_preferred_buffer_size);
        switch (readerPtr->Read(&nRead, buffer.get(), 1, system_preferred_buffer_size)) {
        case 0:
            ofs.write(buffer.get(), (streamsize)nRead);
            continue;
        case -1:
            return -1;
        case -2: // Read at EOF
            break;
        }
        break;
    }

    if (Close(readerPtr->GetHandle())) {
        return -1;
    }
    return 0;
}

int CopyFromLocal(const std::string &sourcefilename, const std::string &destfilename) {
    ServiceRequest request;
    if (ParseUrl(&request, destfilename)) {
        return -1;
    }
    FileStream *writerPtr;
    if (OpenForWriting(&writerPtr, destfilename)) {
        return -1;
    }
    std::unique_ptr<char[]> buffer = std::make_unique<char[]>(system_preferred_buffer_size);
    size_t nRead;
    ifstream ifs(sourcefilename, ios::binary);

    for (;;) {
        GetLogger()->trace("Copying at most {} bytes from local file to remote...", nPreferredBufferSize);
        ifs.read(buffer.get(), nPreferredBufferSize);
        nRead = (size_t)ifs.gcount();
        if (nRead == 0) {
            break;
        }
        int nWriteStatus;
        size_t nWritten;
        if ((nWriteStatus = writerPtr->Write(&nWritten, buffer.get(), 1, nRead))) {
            return nWriteStatus;
        }
    }

    if (Close(writerPtr->GetHandle())) {
        return -1;
    }
    return 0;
}

int Concat(const std::string &destfilename, const std::vector<std::string> &sourcefilenames) {
    GetLogger()->error("Not implemented!");
    return -1;
}

int ComposeMultifile(const std::string &sDestFilePathName, const std::vector<std::string> &sSourceFilePathNames) {
    GetLogger()->error("Not implemented!");
    return -1;
}

}