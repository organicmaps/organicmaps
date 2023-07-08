
#include <cstdlib>
#include <string>

inline std::string with_data_dir(const char* filename) {
    const char* data_dir = getenv("OSMIUM_TEST_DATA_DIR");

    std::string result;
    if (data_dir) {
        result = data_dir;
        result += '/';
    }

    result += filename;

    return result;
}

