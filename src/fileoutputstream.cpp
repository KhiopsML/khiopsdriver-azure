#include "fileoutputstream.hpp"

namespace az
{
	void FileOutputStream::Flush()
	{
		// Do nothing
	}

	FileOutputStream::FileOutputStream() :
		FileStream(),
		nCurrentPos(0)
	{
	}
}
