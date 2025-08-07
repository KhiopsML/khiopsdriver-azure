#pragma once

namespace az
{
	class FileStreamHandle;
}

#include <chrono>
#include <cstddef>

namespace az
{
	class FileStreamHandle
	{
	public:
		struct Hash;

		FileStreamHandle();
		FileStreamHandle(void* voidPtr);

		explicit operator void*() const;

		friend bool operator==(const FileStreamHandle& a, const FileStreamHandle& b);

		struct Hash
		{
			size_t operator()(const FileStreamHandle& fileStreamHandle) const;
		};

	private:
		std::chrono::steady_clock::rep handle;
	};
}
