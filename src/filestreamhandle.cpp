#include "filestreamhandle.hpp"

using namespace std;

namespace az
{
	FileStreamHandle::FileStreamHandle() :
		handle(chrono::steady_clock::now().time_since_epoch().count())
	{
	}

	FileStreamHandle::FileStreamHandle(void* voidPtr) :
		handle((std::chrono::steady_clock::rep)voidPtr)
	{
	}

	explicit FileStreamHandle::operator void*() const
	{
		return (void*)handle;
	}
}
