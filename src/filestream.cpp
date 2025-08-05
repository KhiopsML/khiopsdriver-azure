#include "filestream.hpp"

namespace az
{
	const FileStreamHandle& FileStream::GetHandle() const
	{
		return handle;
	}

	FileStream::FileStream() :
		handle(FileStreamHandle())
	{
	}
}
