#include "reader.h"

namespace transport_catalogue::from {

const VoidPrintDriver& VoidPrintDriver::GetDriver() {
    static VoidPrintDriver driver;
    return driver;
}

VoidPrintDriver::VoidPrintDriver() noexcept
        : into::PrintDriver(std::cerr) {
}

void VoidPrintDriver::PrintObject(std::type_index, const void*) const {}

} // namespace transport_catalogue::from