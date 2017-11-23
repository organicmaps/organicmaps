project(indexer)

include_directories(${OMIM_ROOT}/3party/protobuf/protobuf/src)

set(
  SRC
  altitude_loader.cpp
  altitude_loader.hpp
  categories_holder.cpp
  categories_holder.hpp
  categories_holder_loader.cpp
  categories_index.cpp
  categories_index.hpp
  cell_coverer.hpp
  cell_id.hpp
  centers_table.cpp
  centers_table.hpp
  cities_boundaries_serdes.hpp
  city_boundary.hpp
  classificator.cpp
  classificator_loader.cpp
  classificator_loader.hpp
  classificator.hpp
  coding_params.cpp
  coding_params.hpp
  cuisines.cpp
  cuisines.hpp
  data_factory.cpp
  data_factory.hpp
  data_header.cpp
  data_header.hpp
  drawing_rule_def.cpp
  drawing_rule_def.hpp
  drawing_rules.cpp
  drawing_rules.hpp
  drules_include.hpp
  drules_selector_parser.cpp
  drules_selector_parser.hpp
  drules_selector.cpp
  drules_selector.hpp
  drules_struct.pb.cc
  drules_struct.pb.h
  editable_map_object.cpp
  editable_map_object.hpp
  edits_migration.cpp
  edits_migration.hpp
  fake_feature_ids.cpp
  fake_feature_ids.hpp
  feature_algo.cpp
  feature_algo.hpp
  feature_altitude.hpp
  feature_covering.cpp
  feature_covering.hpp
  feature_data.cpp
  feature_data.hpp
  feature_decl.cpp
  feature_decl.hpp
  feature_impl.cpp
  feature_impl.hpp
  feature_loader_base.cpp
  feature_loader_base.hpp
  feature_loader.cpp
  feature_loader.hpp
  feature_meta.cpp
  feature_meta.hpp
  feature_processor.hpp
  feature_utils.cpp
  feature_utils.hpp
  feature_visibility.cpp
  feature_visibility.hpp
  feature.cpp
  feature.hpp
  features_offsets_table.cpp
  features_offsets_table.hpp
  features_vector.cpp
  features_vector.hpp
  ftraits.hpp
  ftypes_matcher.cpp
  ftypes_matcher.hpp
  ftypes_sponsored.cpp
  ftypes_sponsored.hpp
  geometry_coding.cpp
  geometry_coding.hpp
  geometry_serialization.cpp
  geometry_serialization.hpp
  index_builder.cpp
  index_builder.hpp
  index_helpers.cpp
  index_helpers.hpp
  index.cpp
  index.hpp
  interval_index_builder.hpp
  interval_index_iface.hpp
  interval_index.hpp
  map_object.cpp
  map_object.hpp
  map_style_reader.cpp
  map_style_reader.hpp
  map_style.cpp
  map_style.hpp
  mwm_set.cpp
  mwm_set.hpp
  new_feature_categories.cpp   # it's in indexer because of CategoriesHolder dependency.
  new_feature_categories.hpp   # it's in indexer because of CategoriesHolder dependency.
  old/feature_loader_101.cpp
  old/feature_loader_101.hpp
  old/interval_index_101.hpp
  osm_editor.cpp
  osm_editor.hpp
  postcodes_matcher.cpp   # it's in indexer due to editor which is in indexer and depends on postcodes_marcher
  postcodes_matcher.hpp    # it's in indexer due to editor which is in indexer and depends on postcodes_marcher
  rank_table.cpp
  rank_table.hpp
  road_shields_parser.cpp
  road_shields_parser.hpp
  scale_index_builder.hpp
  scale_index.hpp
  scales.cpp
  scales.hpp
  scales_patch.hpp
  search_delimiters.cpp     # it's in indexer because of CategoriesHolder dependency.
  search_delimiters.hpp       # it's in indexer because of CategoriesHolder dependency.
  search_string_utils.cpp   # it's in indexer because of CategoriesHolder dependency.
  search_string_utils.hpp     # it's in indexer because of CategoriesHolder dependency.
  string_set.hpp
  string_slice.cpp
  string_slice.hpp
  succinct_trie_builder.hpp
  succinct_trie_reader.hpp
  tesselator_decl.hpp
  tree_structure.hpp
  trie_builder.hpp
  trie_reader.hpp
  trie.hpp
  types_mapping.cpp
  types_mapping.hpp
  unique_index.hpp
)

set(
  OTHER_FILES
  drules_struct.proto
)

file(COPY ${OTHER_FILES} DESTINATION ${CMAKE_BINARY_DIR})

add_library(${PROJECT_NAME} ${SRC})

omim_add_test_subdirectory(indexer_tests_support)
omim_add_test_subdirectory(indexer_tests)
