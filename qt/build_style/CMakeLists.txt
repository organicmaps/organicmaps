omim_add_library(qt_designer
  build_common.cpp
  build_common.h
  build_drules.cpp
  build_drules.h
  build_skins.cpp
  build_skins.h
  build_statistics.cpp
  build_statistics.h
  build_style.cpp
  build_style.h
  run_tests.cpp
  run_tests.h
)

target_link_libraries(qt_designer PUBLIC indexer platform skin_generator Qt6::Core)
