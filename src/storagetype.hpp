#pragma once

#include <string>
#include <sstream>
#include "exception.hpp"

namespace az
{
    enum class StorageType
    {
        BLOB,
        SHARE
    };
	
	class UnknownStorageTypeError : public Error
	{
	public:
		inline UnknownStorageTypeError(StorageType storageType) :
			sMessage((std::ostringstream() << "unknown storage type '" << (int)storageType << "'").str())
		{
		}

		inline virtual const char* what() const noexcept override
		{
			return sMessage.c_str();
		}

	private:
		std::string sMessage;
	};
}
