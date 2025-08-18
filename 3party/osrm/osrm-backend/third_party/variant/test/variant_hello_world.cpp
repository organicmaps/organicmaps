#include "variant.hpp"
#include <cstdint>
#include <stdexcept>

using namespace mapbox;

struct check : util::static_visitor<>
{
    template <typename T>
    void operator() (T const& val) const
    {
        if (val != 0) throw std::runtime_error("invalid");
    }
};


int main() {
    typedef util::variant<bool, int, double> variant_type;
    variant_type v(0);
    util::apply_visitor(check(), v);
    return 0;
}
