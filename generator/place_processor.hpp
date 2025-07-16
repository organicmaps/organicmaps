#pragma once

#include "generator/cities_boundaries_builder.hpp"
#include "generator/collector_routing_city_boundaries.hpp"
#include "generator/feature_builder.hpp"

#include "base/geo_object_id.hpp"

#include <map>
#include <string>
#include <vector>

namespace generator
{
struct FeaturePlace
{
  feature::FeatureBuilder m_fb;
  m2::RectD m_rect;

  uint8_t GetRank() const { return m_fb.GetRank(); }
  uint32_t GetPlaceType() const;
  bool IsRealCapital() const;
};

// The class PlaceProcessor is responsible for the union of boundaries of the places.
class PlaceProcessor
{
public:
  explicit PlaceProcessor(std::string const & filename);

  void Add(feature::FeatureBuilder && fb);

  using IDsContainerT = std::vector<base::GeoObjectId>;
  /// @param[out] ids To store correspondent IDs for test purposes if not nullptr.
  std::vector<feature::FeatureBuilder> ProcessPlaces(std::vector<IDsContainerT> * ids = nullptr);

  OsmIdToBoundariesTable & GetTable() { return m_boundariesTable; }

private:
  PlaceBoundariesHolder m_boundariesHolder;

  FeaturePlace CreatePlace(feature::FeatureBuilder && fb) const;

  struct Key
  {
    int m_idx;
    std::string m_name;

    bool operator<(Key const & rhs) const
    {
      if ((m_idx >= 0 && m_idx == rhs.m_idx) || m_name == rhs.m_name)
        return false;

      return m_name < rhs.m_name;
    }
  };
  std::map<Key, std::vector<FeaturePlace>> m_nameToPlaces;

  OsmIdToBoundariesTable m_boundariesTable;

  std::string m_logTag;
};
}  // namespace generator
