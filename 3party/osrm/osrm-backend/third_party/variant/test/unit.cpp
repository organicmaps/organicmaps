#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

#include "variant.hpp"
#include "variant_io.hpp"
#include <algorithm>
#include <cstdint>
#include <iterator>
#include <limits>
#include <memory>
#include <ostream>
#include <sstream>
#include <string>

using namespace mapbox;

template <typename T>
struct mutating_visitor
{
    mutating_visitor(T & val)
        : val_(val) {}

    void operator() (T & val) const
    {
        val = val_;
    }

    template <typename T1>
    void operator() (T1& ) const {} // no-op

    T & val_;
};



TEST_CASE( "variant version", "[variant]" ) {
    unsigned int version = VARIANT_VERSION;
    REQUIRE(version == 100);
    #if VARIANT_VERSION == 100
        REQUIRE(true);
    #else
        REQUIRE(false);
    #endif
}

TEST_CASE( "variant can be moved into vector", "[variant]" ) {
    typedef util::variant<bool,std::string> variant_type;
    variant_type v(std::string("test"));
    std::vector<variant_type> vec;
    vec.emplace_back(std::move(v));
    REQUIRE(v.get<std::string>() != std::string("test"));
    REQUIRE(vec.at(0).get<std::string>() == std::string("test"));
}

TEST_CASE( "variant should support built-in types", "[variant]" ) {
    SECTION( "bool" ) {
        util::variant<bool> v(true);
        REQUIRE(v.valid());
        REQUIRE(v.is<bool>());
        REQUIRE(v.get_type_index() == 0);
        REQUIRE(v.get<bool>() == true);
        v.set<bool>(false);
        REQUIRE(v.get<bool>() == false);
        v = true;
        REQUIRE(v == util::variant<bool>(true));
    }
    SECTION( "nullptr" ) {
        typedef std::nullptr_t value_type;
        util::variant<value_type> v(nullptr);
        REQUIRE(v.valid());
        REQUIRE(v.is<value_type>());
        REQUIRE(v.get_type_index() == 0);
        // TODO: commented since it breaks on windows: 'operator << is ambiguous'
        //REQUIRE(v.get<value_type>() == nullptr);
        // FIXME: does not compile: ./variant.hpp:340:14: error: use of overloaded operator '<<' is ambiguous (with operand types 'std::__1::basic_ostream<char>' and 'const nullptr_t')
        // https://github.com/mapbox/variant/issues/14
        //REQUIRE(v == util::variant<value_type>(nullptr));
    }
    SECTION( "unique_ptr" ) {
        typedef std::unique_ptr<std::string> value_type;
        util::variant<value_type> v(value_type(new std::string("hello")));
        REQUIRE(v.valid());
        REQUIRE(v.is<value_type>());
        REQUIRE(v.get_type_index() == 0);
        REQUIRE(*v.get<value_type>().get() == *value_type(new std::string("hello")).get());
    }
    SECTION( "string" ) {
        typedef std::string value_type;
        util::variant<value_type> v(value_type("hello"));
        REQUIRE(v.valid());
        REQUIRE(v.is<value_type>());
        REQUIRE(v.get_type_index() == 0);
        REQUIRE(v.get<value_type>() == value_type("hello"));
        v.set<value_type>(value_type("there"));
        REQUIRE(v.get<value_type>() == value_type("there"));
        v = value_type("variant");
        REQUIRE(v == util::variant<value_type>(value_type("variant")));
    }
    SECTION( "size_t" ) {
        typedef std::size_t value_type;
        util::variant<value_type> v(std::numeric_limits<value_type>::max());
        REQUIRE(v.valid());
        REQUIRE(v.is<value_type>());
        REQUIRE(v.get_type_index() == 0);
        REQUIRE(v.get<value_type>() == std::numeric_limits<value_type>::max());
        v.set<value_type>(value_type(0));
        REQUIRE(v.get<value_type>() == value_type(0));
        v = value_type(1);
        REQUIRE(v == util::variant<value_type>(value_type(1)));
    }
    SECTION( "int8_t" ) {
        typedef std::int8_t value_type;
        util::variant<value_type> v(std::numeric_limits<value_type>::max());
        REQUIRE(v.valid());
        REQUIRE(v.is<value_type>());
        REQUIRE(v.get_type_index() == 0);
        REQUIRE(v.get<value_type>() == std::numeric_limits<value_type>::max());
        v.set<value_type>(0);
        REQUIRE(v.get<value_type>() == value_type(0));
        v = value_type(1);
        REQUIRE(v == util::variant<value_type>(value_type(1)));
    }
    SECTION( "int16_t" ) {
        typedef std::int16_t value_type;
        util::variant<value_type> v(std::numeric_limits<value_type>::max());
        REQUIRE(v.valid());
        REQUIRE(v.is<value_type>());
        REQUIRE(v.get_type_index() == 0);
        REQUIRE(v.get<value_type>() == std::numeric_limits<value_type>::max());
        v.set<value_type>(0);
        REQUIRE(v.get<value_type>() == value_type(0));
        v = value_type(1);
        REQUIRE(v == util::variant<value_type>(value_type(1)));
    }
    SECTION( "int32_t" ) {
        typedef std::int32_t value_type;
        util::variant<value_type> v(std::numeric_limits<value_type>::max());
        REQUIRE(v.valid());
        REQUIRE(v.is<value_type>());
        REQUIRE(v.get_type_index() == 0);
        REQUIRE(v.get<value_type>() == std::numeric_limits<value_type>::max());
        v.set<value_type>(0);
        REQUIRE(v.get<value_type>() == value_type(0));
        v = value_type(1);
        REQUIRE(v == util::variant<value_type>(value_type(1)));
    }
    SECTION( "int64_t" ) {
        typedef std::int64_t value_type;
        util::variant<value_type> v(std::numeric_limits<value_type>::max());
        REQUIRE(v.valid());
        REQUIRE(v.is<value_type>());
        REQUIRE(v.get_type_index() == 0);
        REQUIRE(v.get<value_type>() == std::numeric_limits<value_type>::max());
        v.set<value_type>(0);
        REQUIRE(v.get<value_type>() == value_type(0));
        v = value_type(1);
        REQUIRE(v == util::variant<value_type>(value_type(1)));
    }
}

struct MissionInteger
{
    typedef uint64_t value_type;
    value_type val_;
    public:
      MissionInteger(uint64_t val) :
       val_(val) {}

    bool operator==(MissionInteger const& rhs) const
    {
        return (val_ == rhs.get());
    }

    uint64_t get() const
    {
        return val_;
    }
};

// TODO - remove after https://github.com/mapbox/variant/issues/14
std::ostream& operator<<(std::ostream& os, MissionInteger const& rhs)
{
    os << rhs.get();
    return os;
}

TEST_CASE( "variant should support custom types", "[variant]" ) {
    // http://www.missionintegers.com/integer/34838300
    util::variant<MissionInteger> v(MissionInteger(34838300));
    REQUIRE(v.valid());
    REQUIRE(v.is<MissionInteger>());
    REQUIRE(v.get_type_index() == 0);
    REQUIRE(v.get<MissionInteger>() == MissionInteger(34838300));
    REQUIRE(v.get<MissionInteger>().get() == MissionInteger::value_type(34838300));
    // TODO: should both of the set usages below compile?
    v.set<MissionInteger>(MissionInteger::value_type(0));
    v.set<MissionInteger>(MissionInteger(0));
    REQUIRE(v.get<MissionInteger>().get() == MissionInteger::value_type(0));
    v = MissionInteger(1);
    REQUIRE(v == util::variant<MissionInteger>(MissionInteger(1)));
}

// Test internal api
TEST_CASE( "variant should correctly index types", "[variant]" ) {
    typedef util::variant<bool,std::string,std::uint64_t,std::int64_t,double,float> variant_type;
    // Index is in reverse order
    REQUIRE(variant_type(true).get_type_index() == 5);
    REQUIRE(variant_type(std::string("test")).get_type_index() == 4);
    REQUIRE(variant_type(std::uint64_t(0)).get_type_index() == 3);
    REQUIRE(variant_type(std::int64_t(0)).get_type_index() == 2);
    REQUIRE(variant_type(double(0.0)).get_type_index() == 1);
    REQUIRE(variant_type(float(0.0)).get_type_index() == 0);
}

// Test internal api
TEST_CASE( "variant::which() returns zero based index of stored type", "[variant]" ) {
    typedef util::variant<bool,std::string,std::uint64_t,std::int64_t,double,float> variant_type;
    // Index is in reverse order
    REQUIRE(variant_type(true).which() == 0);
    REQUIRE(variant_type(std::string("test")).which() == 1);
    REQUIRE(variant_type(std::uint64_t(0)).which() == 2);
    REQUIRE(variant_type(std::int64_t(0)).which() == 3);
    REQUIRE(variant_type(double(0.0)).which() == 4);
    REQUIRE(variant_type(float(0.0)).which() == 5);
}

TEST_CASE( "get with type not in variant type list should throw", "[variant]" ) {
    typedef util::variant<int> variant_type;
    variant_type var = 5;
    REQUIRE(var.get<int>() == 5);
}

TEST_CASE( "get with wrong type (here: double) should throw", "[variant]" ) {
    typedef util::variant<int, double> variant_type;
    variant_type var = 5;
    REQUIRE(var.get<int>() == 5);
    REQUIRE_THROWS(var.get<double>());
}

TEST_CASE( "get with wrong type (here: int) should throw", "[variant]" ) {
    typedef util::variant<int, double> variant_type;
    variant_type var = 5.0;
    REQUIRE(var.get<double>() == 5.0);
    REQUIRE_THROWS(var.get<int>());
}

TEST_CASE( "implicit conversion", "[variant][implicit conversion]" ) {
    typedef util::variant<int> variant_type;
    variant_type var(5.0); // converted to int
    REQUIRE(var.get<int>() == 5);
    var = 6.0; // works for operator=, too
    REQUIRE(var.get<int>() == 6);
}

TEST_CASE( "implicit conversion to first type in variant type list", "[variant][implicit conversion]" ) {
    typedef util::variant<long, char> variant_type;
    variant_type var = 5.0; // converted to long
    REQUIRE(var.get<long>() == 5);
    REQUIRE_THROWS(var.get<char>());
}

TEST_CASE( "implicit conversion to unsigned char", "[variant][implicit conversion]" ) {
    typedef util::variant<unsigned char> variant_type;
    variant_type var = 100.0;
    CHECK(var.get<unsigned char>() == static_cast<unsigned char>(100.0));
    CHECK(var.get<unsigned char>() == static_cast<unsigned char>(static_cast<unsigned int>(100.0)));
}

struct dummy {};

TEST_CASE( "variant value traits", "[variant::detail]" ) {
    // Users should not create variants with duplicated types
    // however our type indexing should still work
    // Index is in reverse order
    REQUIRE((util::detail::value_traits<bool, bool, int, double, std::string>::index == 3));
    REQUIRE((util::detail::value_traits<int, bool, int, double, std::string>::index == 2));
    REQUIRE((util::detail::value_traits<double, bool, int, double, std::string>::index == 1));
    REQUIRE((util::detail::value_traits<std::string, bool, int, double, std::string>::index == 0));
    REQUIRE((util::detail::value_traits<dummy, bool, int, double, std::string>::index == util::detail::invalid_value));
    REQUIRE((util::detail::value_traits<std::vector<int>, bool, int, double, std::string>::index == util::detail::invalid_value));
}

TEST_CASE( "variant default constructor", "[variant][default constructor]" ) {
    // By default variant is initialised with (default constructed) first type in template parameters pack
    // As a result first type in Types... must be default constructable
    // NOTE: index in reverse order -> index = N - 1
    REQUIRE((util::variant<int, double, std::string>().get_type_index() == 2));
    REQUIRE((util::variant<int, double, std::string>(util::no_init()).get_type_index() == util::detail::invalid_value));
}

TEST_CASE( "variant visitation", "[visitor][unary visitor]" ) {
    util::variant<int, double, std::string> var(123);
    REQUIRE(var.get<int>() == 123);
    int val = 456;
    mutating_visitor<int> visitor(val);
    util::apply_visitor(visitor, var);
    REQUIRE(var.get<int>() == 456);
}

TEST_CASE( "variant printer", "[visitor][unary visitor][printer]" ) {
    typedef util::variant<int, double, std::string> variant_type;
    std::vector<variant_type> var = {2.1, 123, "foo", 456};
    std::stringstream out;
    std::copy(var.begin(), var.end(), std::ostream_iterator<variant_type>(out, ","));
    out << var[2];
    REQUIRE(out.str() == "2.1,123,foo,456,foo");
}

int main (int argc, char* const argv[])
{
    int result = Catch::Session().run(argc, argv);
    if (!result) printf("\x1b[1;32m ✓ \x1b[0m\n");
    return result;
}
