#include "storagetype.hpp"
#include <exception>
#include <sstream>

using namespace std;

void PrintTo(const StorageType& storageType, std::ostream* os)
{
    switch (storageType)
    {
    case StorageType::BLOB:
        *os << "Blob";
        break;
    case StorageType::SHARE:
        *os << "Share";
        break;
    default:
        throw invalid_argument((ostringstream() << "invalid storage type" << (int)storageType).str());
    }
}
