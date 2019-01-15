if (NOT PYBINDINGS_VERSION)
    message(FATAL_ERROR "You need to specify PYBINDINGS_VERSION to build PYBINDINGS")
endif()

configure_file(module_version.hpp.in module_version.hpp)
