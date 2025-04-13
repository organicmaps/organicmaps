#pragma once

#include "generator/composite_id.hpp"

#include "indexer/classificator.hpp"
#include "indexer/complex/tree_node.hpp"
#include "indexer/feature_data.hpp"

#include "storage/storage_defines.hpp"

#include "geometry/point2d.hpp"

#include "coding/csv_reader.hpp"

#include <cstdint>
#include <optional>
#include <string>

namespace generator
{
struct HierarchyEntry
{
  CompositeId m_id;
  std::optional<CompositeId> m_parentId;
  size_t m_depth = 0;
  std::string m_name;
  storage::CountryId m_country;
  m2::PointD m_center;
  uint32_t m_type = ftype::GetEmptyValue();
};

bool operator==(HierarchyEntry const & lhs, HierarchyEntry const & rhs);

std::string DebugPrint(HierarchyEntry const & entry);

namespace hierarchy
{
static char const kCsvDelimiter = ';';

uint32_t GetMainType(FeatureParams::Types const & types);
std::string GetName(StringUtf8Multilang const & str);

coding::CSVReader::Row HierarchyEntryToCsvRow(HierarchyEntry const & entry);
HierarchyEntry HierarchyEntryFromCsvRow(coding::CSVReader::Row const & row);

std::string HierarchyEntryToCsvString(HierarchyEntry const & entry, char delim = kCsvDelimiter);

tree_node::types::Ptrs<HierarchyEntry> LoadHierachy(std::string const & filename);
}  // namespace hierarchy
}  // namespace generator
