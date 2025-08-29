#pragma once

namespace az
{
	class FileStream;
}

#include "filestreamhandle.hpp"

namespace az
{
	class FileStream
	{
	public:
		virtual ~FileStream();
		const FileStreamHandle& GetHandle() const;
		virtual void Close() = 0;

	protected:
		FileStream();

	private:
		FileStreamHandle handle;
	};
}
