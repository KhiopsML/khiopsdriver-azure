#pragma once

namespace az
{
	class Error;
	class InvalidDomainError;
	class NotConnectedError;
	class IncompatibleConnectionStringError;
	class NetworkError;
	class InvalidUrlError;
	class InvalidFileUrlPathError;
	class InvalidDirUrlPathError;
	enum class Operation;
	class InvalidOperationForDirError;
	class NoFileError;
	class DeletionError;
	class InvalidFileStreamModeError;
	class InvalidSeekOriginError;
	class InvalidSeekOffsetError;
	class FileStreamNotFoundError;
	class IntermediateDirNotFoundError;
	class DirAlreadyExistsError;
	class CreationError;
}

#include <exception>
#include <string>

namespace az
{
	class Error: public std::exception
	{
	};

	class NullArgError : public Error
	{
	public:
		NullArgError(const char* sFuncname, const char* sArgname);

		virtual const char* what() const noexcept override;

	private:
		std::string sMessage;
	};

	class InvalidDomainError : public Error
	{
	public:
		InvalidDomainError(const std::string& sDomain);

		virtual const char* what() const noexcept override;

	private:
		std::string sMessage;
	};

	class NotConnectedError : public Error
	{
	public:
		virtual const char* what() const noexcept override;
	};

	class IncompatibleConnectionStringError : public Error
	{
	public:
		virtual const char* what() const noexcept override;
	};

	class NetworkError : public Error
	{
	public:
		virtual const char* what() const noexcept override;
	};

	class InvalidUrlError : public Error
	{
	public:
		InvalidUrlError(const std::string& sUrl);

		virtual const char* what() const noexcept override;

	private:
		std::string sMessage;
	};

	class InvalidFileUrlPathError : public Error
	{
	public:
		InvalidFileUrlPathError(const std::string& sPath);

		virtual const char* what() const noexcept override;

	private:
		std::string sMessage;
	};

	class InvalidDirUrlPathError : public Error
	{
	public:
		InvalidDirUrlPathError(const std::string& sPath);

		virtual const char* what() const noexcept override;

	private:
		std::string sMessage;
	};

	enum class Operation
	{
		GET_SIZE,
		READ,
		WRITE,
		APPEND,
		REMOVE
	};

	std::string FormatOperation(Operation operation);

	class InvalidOperationForDirError : public Error
	{
	public:
		InvalidOperationForDirError(Operation operation, std::string sDetails = std::string());

		virtual const char* what() const noexcept override;

	private:
		std::string sMessage;
	};

	class NoFileError : public Error
	{
	public:
		NoFileError(const std::string& sUrl);

		virtual const char* what() const noexcept override;

	private:
		std::string sMessage;
	};

	class DeletionError : public Error
	{
	public:
		DeletionError(const std::string& sUrl);

		virtual const char* what() const noexcept override;

	private:
		std::string sMessage;
	};

	class InvalidFileStreamModeError : public Error
	{
	public:
		InvalidFileStreamModeError(const std::string& sUrl, char mode);

		virtual const char* what() const noexcept override;

	private:
		std::string sMessage;
	};

	class InvalidSeekOriginError : public Error
	{
	public:
		InvalidSeekOriginError(int nOrigin);

		virtual const char* what() const noexcept override;

	private:
		std::string sMessage;
	};

	class InvalidSeekOffsetError : public Error
	{
	public:
		InvalidSeekOffsetError(int nOffset, int nOrigin);

		virtual const char* what() const noexcept override;

	private:
		std::string sMessage;
	};

	class FileStreamNotFoundError : public Error
	{
	public:
		FileStreamNotFoundError(const void* handle);

		virtual const char* what() const noexcept override;

	private:
		std::string sMessage;
	};

	class IntermediateDirNotFoundError : public Error
	{
	public:
		IntermediateDirNotFoundError(const std::string& sUrl);

		virtual const char* what() const noexcept override;

	private:
		std::string sMessage;
	};

	class DirAlreadyExistsError : public Error
	{
	public:
		DirAlreadyExistsError(const std::string& sUrl);

		virtual const char* what() const noexcept override;

	private:
		std::string sMessage;
	};

	class CreationError : public Error
	{
	public:
		CreationError(const std::string& sUrl);

		virtual const char* what() const noexcept override;

	private:
		std::string sMessage;
	};
}
