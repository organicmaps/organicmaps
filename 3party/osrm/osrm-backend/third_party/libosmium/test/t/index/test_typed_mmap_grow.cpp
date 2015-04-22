#include "catch.hpp"

#include <osmium/index/detail/typed_mmap.hpp>

#if defined(_MSC_VER) || (defined(__GNUC__) && defined(_WIN32))
#include "win_mkstemp.hpp"
#endif

TEST_CASE("TypedMmapGrow") {

    SECTION("GrowAndMap") {
        const int size = 100;
        char filename[] = "test_mmap_grow_and_map_XXXXXX";
        const int fd = mkstemp(filename);
        REQUIRE(fd > 0);

        uint64_t* data = osmium::detail::typed_mmap<uint64_t>::grow_and_map(size, fd);
        REQUIRE(size == osmium::detail::typed_mmap<uint64_t>::file_size(fd));

        data[0] = 1ul;
        data[1] = 8ul;
        data[99] = 27ul;

        REQUIRE(1ul == data[0]);
        REQUIRE(8ul == data[1]);
        REQUIRE(27ul == data[99]);

        osmium::detail::typed_mmap<uint64_t>::unmap(data, size);

        REQUIRE(0 == close(fd));
        REQUIRE(0 == unlink(filename));
    }

}
