# Flags for all
if (MSVC)
  set(OMIM_WARNING_FLAGS /W0)
  add_compile_options(/Zc:lambda)
  add_compile_options(/Zc:externConstexpr)
else()
  set(OMIM_WARNING_FLAGS
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wno-unused-parameter>  # We have a lot of functions with unused parameters
  )
endif()

set(3PARTY_INCLUDE_DIRS "${OMIM_ROOT}/3party/boost" "${OMIM_ROOT}/3party/utf8cpp/include")
set(OMIM_DATA_DIR "${OMIM_ROOT}/data")
set(OMIM_USER_RESOURCES_DIR "${OMIM_ROOT}/data")
