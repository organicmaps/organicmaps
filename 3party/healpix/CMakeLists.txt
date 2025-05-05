project(healpix)

set(SRC
  healpix/src/cxx/cxxsupport/announce.cc
  healpix/src/cxx/cxxsupport/geom_utils.cc
  healpix/src/cxx/cxxsupport/string_utils.cc
  healpix/src/cxx/cxxsupport/ls_image.cc
  healpix/src/cxx/cxxsupport/paramfile.cc
  healpix/src/cxx/cxxsupport/pointing.cc
  healpix/src/cxx/cxxsupport/rotmatrix.cc
  healpix/src/cxx/cxxsupport/trafos.cc
  healpix/src/cxx/cxxsupport/walltimer.cc
  healpix/src/cxx/cxxsupport/wigner.cc
  healpix/src/cxx/cxxsupport/error_handling.cc
  healpix/src/cxx/cxxsupport/fitshandle.cc
  healpix/src/cxx/Healpix_cxx/alm.cc
  healpix/src/cxx/Healpix_cxx/alm_healpix_tools.cc
  healpix/src/cxx/Healpix_cxx/alm_powspec_tools.cc
  healpix/src/cxx/Healpix_cxx/healpix_tables.cc
  healpix/src/cxx/Healpix_cxx/healpix_base.cc
  healpix/src/cxx/Healpix_cxx/healpix_map.cc
  healpix/src/cxx/Healpix_cxx/powspec.cc
  healpix/src/cxx/Healpix_cxx/moc_query.cc
  healpix/src/cxx/Healpix_cxx/weight_utils.cc
  healpix/src/cxx/Healpix_cxx/mask_tools.cc
  healpix/src/cxx/Healpix_cxx/alice3.cc
  healpix/src/cxx/Healpix_cxx/healpix_data_io.cc
  healpix/src/cxx/Healpix_cxx/healpix_map_fitsio.cc
  healpix/src/cxx/Healpix_cxx/alm_fitsio.cc
  healpix/src/cxx/Healpix_cxx/powspec_fitsio.cc
  healpix/src/cxx/Healpix_cxx/moc_fitsio.cc
)

set(TESTS OFF)
add_subdirectory(cfitsio)

add_library(${PROJECT_NAME} ${SRC})

target_include_directories(${PROJECT_NAME} PUBLIC healpix/src/cxx/cxxsupport healpix/src/cxx/Healpix_cxx)
target_link_libraries(${PROJECT_NAME} PUBLIC cfitsio)