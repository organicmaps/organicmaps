#include <boost/variant.hpp>
#include <cstdint>
#include <stdexcept>

struct check : boost::static_visitor<>
{
    template <typename T>
    void operator() (T const& val) const
    {
        if (val != 0) throw std::runtime_error("invalid");
    }
};

int main() {
    typedef boost::variant<bool, int, double> variant_type;
    variant_type v(0);
    boost::apply_visitor(check(), v);
    return 0;
}
