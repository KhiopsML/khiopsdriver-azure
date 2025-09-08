#include "filestream.hpp"
#include <chrono>

using namespace std;

namespace az
{
	FileStream::~FileStream()
	{
	}

	FileStream::operator void* () const
	{
		return handle;
	}

	FileStream::FileStream() :
		handle((void*)chrono::steady_clock::now().time_since_epoch().count())
	{
	}
}
