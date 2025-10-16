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
    throw invalid_argument(
        (ostringstream() << "invalid storage type" << (int)storageType).str());
  }
}
