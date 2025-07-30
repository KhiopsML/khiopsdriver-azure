#pragma once

class UriProvider;

#include <string>

class UriProvider
{
public:
	UriProvider(bool bIsEmulatedStorage);

	std::string sInexistantDir;
	std::string sDir;
	std::string sInexistantFile;
	std::string sFile;
	std::string sStarGlobFile;
};
