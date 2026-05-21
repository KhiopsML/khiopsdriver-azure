#include "khiops_driver_common/backend.hpp"
#include "khiops_driver_azure/driver2.hpp"
#include <memory>

using namespace std;

namespace khiops_driver_common {

const Backend *GetBackend() {
    static unique_ptr<Backend> backend = nullptr;
    if (backend == nullptr) {
        backend = make_unique<Backend>();
        backend->GetDriverName = &khiops_driver_azure::GetDriverName;
    }
    return backend.get();
}

}

namespace khiops_driver_azure {

int GetDriverName(std::string *result) {
    *result = "Azure driver";
    return 0;
}

}