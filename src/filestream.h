#pragma once

namespace az
{
	class FileStream;
}

namespace az
{
	class FileStream
	{
	public:
		void* GetHandle() const;
		void Close();
		size_t Read(void* dest, size_t size, size_t count);
		void Seek(long long int offset, int whence);
		size_t Write(const void* source, size_t size, size_t count);
		void Flush();
	};
}
