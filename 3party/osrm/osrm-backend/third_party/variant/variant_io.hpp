#ifndef MAPBOX_UTIL_VARIANT_IO_HPP
#define MAPBOX_UTIL_VARIANT_IO_HPP

namespace mapbox { namespace util {

namespace detail {
// operator<< helper
template <typename Out>
class printer
{
public:
    explicit printer(Out & out)
        : out_(out) {}
    printer& operator=(printer const&) = delete;

// visitor
    template <typename T>
    void operator()(T const& operand) const
    {
        out_ << operand;
    }
private:
    Out & out_;
};
}

// operator<<
template <typename charT, typename traits, typename... Types>
VARIANT_INLINE std::basic_ostream<charT, traits>&
operator<< (std::basic_ostream<charT, traits>& out, variant<Types...> const& rhs)
{
    detail::printer<std::basic_ostream<charT, traits>> visitor(out);
    apply_visitor(visitor, rhs);
    return out;
}

}}

#endif //MAPBOX_UTIL_VARIANT_IO_HPP
