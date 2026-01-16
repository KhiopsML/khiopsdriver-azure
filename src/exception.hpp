// Exceptions thrown by the driver. They are caught by the C-interface so they
// do not propagate to user code.

#pragma once

#include <exception>
#include <sstream>
#include <string>

// Macro added to fix quickly compilation errors on old Linux distros.
// Anyway, this file will be deleted because, to conform with our development
// policies, we should not throw any exceptions.
#define concatenate(what)                                                      \
  [=]() {                                                                      \
    std::ostringstream oss;                                                    \
    oss << what;                                                               \
    return oss.str();                                                          \
  }()

namespace az {

class Error : public std::exception {
public:
  inline Error(std::string sMessage) : sMessage(sMessage) {}
  inline virtual const char *what() const noexcept override {
    return sMessage.c_str();
  };

protected:
  std::string sMessage;
};

class NullArgError : public Error {
public:
  inline NullArgError(const char *sFuncname, const char *sArgname)
      : Error(concatenate("error passing null pointer as '"
                          << sArgname << "' argument to function '" << sFuncname
                          << "'")) {}
};

class InvalidDomainError : public Error {
public:
  inline InvalidDomainError(const std::string &sDomain)
      : Error(concatenate("invalid domain: " << sDomain)) {}
};

class NotConnectedError : public Error {
public:
  inline NotConnectedError() : Error("not connected") {}
};

class IncompatibleConnectionStringError : public Error {
public:
  inline IncompatibleConnectionStringError()
      : Error("connection string is not valid for the provided URL") {}
};

class NetworkError : public Error {
public:
  inline NetworkError()
      : Error("failed to communicate with the storage server") {}
};

class InvalidUrlError : public Error {
public:
  inline InvalidUrlError(const std::string &sUrl)
      : Error(concatenate("invalid URL: " << sUrl)) {}
};

class InvalidObjectPathError : public Error {
public:
  inline InvalidObjectPathError(const std::string &sPath)
      : Error(concatenate("invalid object path: " << sPath)) {}
};

enum class FileOperation { MKDIR, RMDIR };

inline std::string FormatOperation(FileOperation operation) {
  switch (operation) {
  case FileOperation::MKDIR:
    return "making directory";
  case FileOperation::RMDIR:
    return "removing directory";
  default:
    throw std::invalid_argument(
        concatenate("invalid FileOperation: " << (int)operation));
  }
}

class InvalidOperationForFileError : public Error {
public:
  inline InvalidOperationForFileError(FileOperation operation)
      : Error(concatenate("files do not support this operation: "
                          << FormatOperation(operation))) {}
};

enum class DirOperation { GET_SIZE, READ, WRITE, APPEND, REMOVE, COPY };

inline std::string FormatOperation(DirOperation operation) {
  switch (operation) {
  case DirOperation::GET_SIZE:
    return "getting size";
  case DirOperation::READ:
    return "reading";
  case DirOperation::WRITE:
    return "writing";
  case DirOperation::APPEND:
    return "appending";
  case DirOperation::REMOVE:
    return "removing (use driver_rmdir instead)";
  case DirOperation::COPY:
    return "copying";
  default:
    throw std::invalid_argument(
        concatenate("invalid DirOperation: " << (int)operation));
  }
}

class InvalidOperationForDirError : public Error {
public:
  inline InvalidOperationForDirError(DirOperation operation)
      : Error(concatenate("directories do not support this operation: "
                          << FormatOperation(operation))) {}
};

class NoFileError : public Error {
public:
  inline NoFileError(const std::string &sUrl)
      : Error(concatenate("no file exists at URL " << sUrl)) {}
};

class DeletionError : public Error {
public:
  inline DeletionError(const std::string &sUrl)
      : Error(concatenate("failed to delete " << sUrl)) {}
};

class InvalidFileStreamModeError : public Error {
public:
  inline InvalidFileStreamModeError(const std::string &sUrl, char mode)
      : Error(concatenate("tried to open file " << sUrl << " with invalid mode "
                                                << mode)) {}
};

class InvalidSeekOriginError : public Error {
public:
  inline InvalidSeekOriginError(int nOrigin)
      : Error(concatenate("tried to seek from invalid origin '" << nOrigin
                                                                << "'")) {}
};

class InvalidSeekOffsetError : public Error {
public:
  inline InvalidSeekOffsetError(long long int nOffset, int nOrigin)
      : Error(concatenate("tried to seek " << nOffset << " bytes from origin '"
                                           << nOrigin
                                           << "' which is outside the file")) {}
};

class FileStreamNotFoundError : public Error {
public:
  inline FileStreamNotFoundError(const void *handle)
      : Error(concatenate("file stream with handle '" << handle
                                                      << "' not found")) {}
};

class IntermediateDirNotFoundError : public Error {
public:
  inline IntermediateDirNotFoundError(const std::string &sUrl)
      : Error(
            concatenate("intermediate directory '" << sUrl << "' not found")) {}
};

class DirAlreadyExistsError : public Error {
public:
  inline DirAlreadyExistsError(const std::string &sUrl)
      : Error(concatenate("directory '" << sUrl << "' already exists")) {}
};

class CreationError : public Error {
public:
  inline CreationError(const std::string &sUrl)
      : Error(concatenate("failed to create " << sUrl)) {}
};

class ReadAtEOFError : public Error {
public:
  inline ReadAtEOFError() : Error("cannot read after end of file") {}
};

class ReadingUpdatedFileError : public Error {
public:
  inline ReadingUpdatedFileError()
      : Error("the file has been updated during the reading") {}
};
} // namespace az
