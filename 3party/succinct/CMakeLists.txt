project(succinct)

set(SRC
  bit_vector.hpp
  bp_vector.cpp
  bp_vector.hpp
  broadword.hpp
  cartesian_tree.hpp
  darray.hpp
  darray64.hpp
  elias_fano_compressed_list.hpp
  elias_fano_list.hpp
  elias_fano.hpp
  forward_enumerator.hpp
  gamma_bit_vector.hpp
  gamma_vector.hpp
  intrinsics.hpp
  mappable_vector.hpp
  mapper.hpp
  nibble_vector.hpp
  rs_bit_vector.cpp
  rs_bit_vector.hpp
  tables.hpp
  topk_vector.hpp
  util.hpp
  vbyte.hpp
)

# Requires boost include.
omim_add_library(${PROJECT_NAME} ${SRC})
