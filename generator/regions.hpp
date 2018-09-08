#pragma once

#include "generator/feature_builder.hpp"
#include "generator/region_info_collector.hpp"

#include "geometry/rect2d.hpp"

#include "coding/multilang_utf8_string.hpp"

#include "base/geo_object_id.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "3party/boost/boost/geometry.hpp"

namespace feature
{
struct GenerateInfo;
}  // namespace feature

namespace generator
{
namespace regions
{
// This is a helper class that is needed to represent the region.
// With this view, further processing is simplified.
struct Region
{
  static uint8_t constexpr kNoRank = 0;

  using Point = FeatureBuilder1::PointSeq::value_type;
  using BoostPoint = boost::geometry::model::point<double, 2, boost::geometry::cs::cartesian>;
  using BoostPolygon = boost::geometry::model::polygon<BoostPoint>;
  using BoostRect = boost::geometry::model::box<BoostPoint>;

  explicit Region(FeatureBuilder1 const & fb, RegionDataProxy const & rd);

  void DeletePolygon();
  // This function will take the following steps:
  // 1. Return the english name if it exists.
  // 2. Return transliteration if it succeeds.
  // 3. Otherwise, return empty string.
  std::string GetEnglishOrTransliteratedName() const;
  std::string GetName(int8_t lang = StringUtf8Multilang::kDefaultCode) const;
  bool IsCountry() const;
  bool HasIsoCode() const;
  std::string GetIsoCode() const;
  bool Contains(Region const & smaller) const;
  bool ContainsRect(Region const & smaller) const;
  double CalculateOverlapPercentage(Region const & other) const;
  // Absolute rank values do not mean anything. But if the rank of the first object is more than the
  // rank of the second object, then the first object is considered more nested.
  uint8_t GetRank() const;
  std::string GetLabel() const;
  BoostPoint GetCenter() const;
  std::shared_ptr<BoostPolygon> const GetPolygon() const;
  BoostRect const & GetRect() const;
  double GetArea() const;
  base::GeoObjectId GetId() const;

private:
  void FillPolygon(FeatureBuilder1 const & fb);

  StringUtf8Multilang m_name;
  RegionDataProxy m_regionData;
  std::shared_ptr<BoostPolygon> m_polygon;
  BoostRect m_rect;
  double m_area;
};

struct Node
{
  using Ptr = std::shared_ptr<Node>;
  using WeakPtr = std::weak_ptr<Node>;
  using PtrList = std::vector<Ptr>;

  explicit Node(Region && region) : m_region(std::move(region)) {}

  void AddChild(Ptr child) { m_children.push_back(child); }
  PtrList const & GetChildren() const { return m_children; }
  PtrList & GetChildren() { return m_children; }
  void SetChildren(PtrList const children) { m_children = children; }
  void RemoveChildren() { m_children.clear(); }
  bool HasChildren() { return m_children.size(); }
  void SetParent(Ptr parent) { m_parent = parent; }
  Ptr GetParent() const { return m_parent.lock(); }
  Region & GetData() { return m_region; }

private:
  Region m_region;
  PtrList m_children;
  WeakPtr m_parent;
};

class ToStringPolicyInterface
{
public:
  virtual std::string ToString(Node::PtrList const & nodePtrList) = 0;
};

// This class is needed to build a hierarchy of regions. We can have several nodes for a region
// with the same name, represented by a multi-polygon (several polygons).
class RegionsBuilder
{
public:
  using Regions = std::vector<Region>;
  using StringsList = std::vector<std::string>;
  using IdStringList = std::vector<std::pair<base::GeoObjectId, std::string>>;
  using CountryTrees = std::multimap<std::string, Node::Ptr>;

  explicit RegionsBuilder(Regions && regions);
  explicit RegionsBuilder(Regions && regions,
                          std::unique_ptr<ToStringPolicyInterface> toStringPolicy);

  Regions const & GetCountries() const;
  StringsList GetCountryNames() const;
  CountryTrees const & GetCountryTrees() const;
  IdStringList ToIdStringList(Node::Ptr tree) const;

private:
  static Node::PtrList MakeSelectedRegionsByCountry(Region const & country,
                                                    Regions const & allRegions);
  static Node::Ptr BuildCountryRegionTree(Region const & country, Regions const & allRegions);
  void MakeCountryTrees(Regions const & regions);

  std::unique_ptr<ToStringPolicyInterface> m_toStringPolicy;
  CountryTrees m_countryTrees;
  Regions m_countries;
};
}  // namespace regions

bool GenerateRegions(feature::GenerateInfo const & genInfo); 
}  // namespace generator
