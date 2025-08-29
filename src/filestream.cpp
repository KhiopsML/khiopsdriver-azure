#include "filestream.hpp"

namespace az
{
	FileStream::~FileStream()
	{
	}

	const FileStreamHandle& FileStream::GetHandle() const
	{
		return handle;
	}

	FileStream::FileStream() :
		handle(FileStreamHandle())
	{
	}
}
