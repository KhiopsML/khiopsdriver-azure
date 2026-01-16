#include "storagetype.hpp"
#include <exception>
#include <sstream>

using namespace std;
using namespace az;

void PrintTo(const StorageType &storageType, std::ostream *os) {
  switch (storageType) {
  case BLOB:
    *os << "Blob";
    break;
  case SHARE:
    *os << "Share";
    break;
  default:
    ostringstream oss;
    oss << "invalid storage type" << (int)storageType;
    throw invalid_argument(oss.str());
  }
}
