#include "filestream.hpp"
#include <chrono>

using namespace std;

namespace az
{
	FileStream::~FileStream()
	{
	}

	void* FileStream::GetHandle() const
	{
		return handle;
	}

	FileStream::FileStream() :
		handle((void*)chrono::steady_clock::now().time_since_epoch().count())
	{
	}
}
