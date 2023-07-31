#pragma once

#include "queries.h"

namespace transport_catalogue::from {

class VoidPrintDriver : public into::PrintDriver {
public:
    static const VoidPrintDriver& GetDriver();
private:
    VoidPrintDriver() noexcept;

    void PrintObject(std::type_index, const void*) const override;
};

template<typename From>
void ProcessQueries(From from, queries::Handler& handler) {
    auto parse_result = ReadQueries(from);
    parse_result.ProcessModifyQueries(handler, VoidPrintDriver::GetDriver());
}

} // namespace transport_catalogue::from