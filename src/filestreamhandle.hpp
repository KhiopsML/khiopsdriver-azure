#pragma once

namespace az
{
	class FileStreamHandle;
}

#include <chrono>

namespace az
{
	class FileStreamHandle
	{
	public:
		FileStreamHandle();
		FileStreamHandle(void* voidPtr);

		explicit operator void*() const;

	private:
		std::chrono::steady_clock::rep handle;
	};
}
