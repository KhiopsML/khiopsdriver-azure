#include "filereader.hpp"

using namespace std;

namespace az
{
	FileReader::~FileReader()
	{
	}

	FileReader::FileReader() :
		FileStream(),
		nCurrentPos(0)
	{
	}
}
