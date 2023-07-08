if (NOT PYBINDINGS_VERSION)
    execute_process(
	COMMAND grep -h -E "propVersionName|CURRENT_PROJECT_VERSION" xcode/common.xcconfig android/gradle.properties
	COMMAND cut -d = -f 2
	COMMAND awk "{print $1}"
	COMMAND sort -Vr
	COMMAND head -1
	WORKING_DIRECTORY "${OMIM_ROOT}"
	OUTPUT_VARIABLE PYBINDINGS_VERSION
	OUTPUT_STRIP_TRAILING_WHITESPACE
    )
endif()

configure_file(module_version.hpp.in module_version.hpp)
