#pragma once

#include "generator/composite_id.hpp"
#include "generator/feature_builder.hpp"
#include "generator/filter_interface.hpp"
#include "generator/gen_mwm_info.hpp"
#include "generator/hierarchy_entry.hpp"
#include "generator/platform_helpers.hpp"
#include "generator/utils.hpp"

#include "indexer/classificator.hpp"
#include "indexer/complex/tree_node.hpp"
#include "indexer/ftypes_matcher.hpp"

#include "storage/storage_defines.hpp"

#include "geometry/point2d.hpp"
#include "geometry/tree4d.hpp"

#include "coding/csv_reader.hpp"

#include "base/assert.hpp"
#include "base/file_name_utils.hpp"
#include "base/geo_object_id.hpp"
#include "base/thread_pool_computational.hpp"

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace generator::hierarchy
{
using GetMainTypeFn = std::function<uint32_t(FeatureParams::Types const &)>;
using GetNameFn = std::function<std::string(StringUtf8Multilang const &)>;
using PrintFn = std::function<std::string(HierarchyEntry const &)>;

// These are dummy functions.
bool FilterFeatureDefault(feature::FeatureBuilder const &);

// The HierarchyPlace class is an abstraction of FeatureBuilder to build a hierarchy of objects.
// It allows you to work with the geometry of points and areas.
class HierarchyPlace
{
public:
  explicit HierarchyPlace(feature::FeatureBuilder const & fb);

  double GetArea() const { return m_area; }
  CompositeId const & GetCompositeId() const { return m_id; }
  StringUtf8Multilang const & GetName() const { return m_name; }
  FeatureParams::Types const & GetTypes() const { return m_types; }
  m2::RectD const & GetLimitRect() const { return m_rect; }
  m2::PointD const & GetCenter() const { return m_center; }
  bool IsPoint() const { return m_isPoint; }

  bool Contains(HierarchyPlace const & smaller) const;

private:
  bool Contains(m2::PointD const & point) const;

  CompositeId m_id;
  StringUtf8Multilang m_name;
  feature::FeatureBuilder::PointSeq m_polygon;
  FeatureParams::Types m_types;
  m2::RectD m_rect;
  double m_area = 0.0;
  bool m_isPoint = false;
  m2::PointD m_center;
};

// The HierarchyLinker class allows you to link nodes into a tree according to the hierarchy.
// The knowledge of geometry only is used here.
class HierarchyLinker
{
public:
  using Node = tree_node::TreeNode<HierarchyPlace>;
  using Tree4d = m4::Tree<Node::Ptr>;

  explicit HierarchyLinker(Node::Ptrs && nodes);

  Node::Ptrs Link();

private:
  static Tree4d MakeTree4d(Node::Ptrs const & nodes);

  Node::Ptr FindPlaceParent(HierarchyPlace const & place);

  Node::Ptrs m_nodes;
  Tree4d m_tree;
};

class HierarchyEntryEnricher
{
public:
  HierarchyEntryEnricher(std::string const & osm2FtIdsPath, std::string const & countryFullPath);

  std::optional<m2::PointD> GetFeatureCenter(CompositeId const & id) const;

private:
  OsmID2FeatureID m_osm2FtIds;
  FeatureGetter m_featureGetter;
};

class HierarchyLinesBuilder
{
public:
  HierarchyLinesBuilder(HierarchyLinker::Node::Ptrs && trees);

  void SetGetMainTypeFunction(GetMainTypeFn const & getMainType);
  void SetGetNameFunction(GetNameFn const & getName);
  void SetCountry(storage::CountryId const & country);
  void SetHierarchyEntryEnricher(std::unique_ptr<HierarchyEntryEnricher> && enricher);

  std::vector<HierarchyEntry> GetHierarchyLines();

private:
  m2::PointD GetCenter(HierarchyLinker::Node::Ptr const & node);
  HierarchyEntry Transform(HierarchyLinker::Node::Ptr const & node);

  HierarchyLinker::Node::Ptrs m_trees;
  GetMainTypeFn m_getMainType;
  GetNameFn m_getName;
  storage::CountryId m_countryName;
  std::unique_ptr<HierarchyEntryEnricher> m_enricher;
};

HierarchyLinker::Node::Ptrs BuildHierarchy(std::vector<feature::FeatureBuilder> && fbs,
                                           GetMainTypeFn const & getMainType,
                                           std::shared_ptr<FilterInterface> const & filter);

// AddChildrenTo adds children to node of tree if fn returns not empty vector of HierarchyPlaces
// for node id.
void AddChildrenTo(HierarchyLinker::Node::Ptrs & trees,
                   std::function<std::vector<HierarchyPlace>(CompositeId const &)> const & fn);

// FlattenBuildingParts transforms trees from
// building
//        |_building-part
//                       |_building-part
// to
// building
//        |_building-part
//        |_building-part
void FlattenBuildingParts(HierarchyLinker::Node::Ptrs & trees);
}  // namespace generator::hierarchy
