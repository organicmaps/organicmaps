
#include <iostream>
#include <string>

#define CATCH_CONFIG_RUNNER

#include "testdata-testcases.hpp"

#include <osmpbf/osmpbf.h>

std::string dirname;

int main(int argc, char* argv[]) {
    const char* testcases_dir = getenv("TESTCASES_DIR");
    if (testcases_dir) {
        dirname = testcases_dir;
        std::cerr << "Running tests from '" << dirname << "' (from TESTCASES_DIR environment variable)\n";
    } else {
        std::cerr << "Please set TESTCASES_DIR environment variable.\n";
        exit(1);
    }

    int result = Catch::Session().run(argc, argv);

    return result;
}

