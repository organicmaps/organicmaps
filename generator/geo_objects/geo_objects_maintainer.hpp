#pragma once

#include "generator/key_value_storage.hpp"

#include "generator/regions/region_info_getter.hpp"

#include "generator/feature_builder.hpp"

#include "indexer/locality_index.hpp"

#include "coding/reader.hpp"

#include "geometry/point2d.hpp"

#include "base/geo_object_id.hpp"

#include <functional>
#include <memory>
#include <utility>
#include <vector>

#include <boost/optional.hpp>

#include "3party/jansson/myjansson.hpp"

namespace generator
{
namespace geo_objects
{
void UpdateCoordinates(m2::PointD const & point, base::JSONPtr & json);

class GeoObjectMaintainer
{
public:
  using RegionInfoGetter = std::function<boost::optional<KeyValue>(m2::PointD const & pathPoint)>;
  using RegionIdGetter = std::function<std::shared_ptr<JsonValue>(base::GeoObjectId id)>;

  struct GeoObjectData
  {
    std::string m_street;
    std::string m_house;
    base::GeoObjectId m_regionId;
  };

  using GeoId2GeoData = std::unordered_map<base::GeoObjectId, GeoObjectData>;
  using GeoIndex = indexer::GeoObjectsIndex<ReaderPtr<Reader>>;

  class GeoObjectsView
  {
  public:
    GeoObjectsView(GeoIndex const & geoIndex, GeoId2GeoData const & geoId2GeoData,
                   RegionIdGetter const & regionIdGetter, std::mutex & updateMutex)
      : m_geoIndex(geoIndex)
      , m_geoId2GeoData(geoId2GeoData)
      , m_regionIdGetter(regionIdGetter)
      , m_lock(updateMutex, std::defer_lock)
    {
      CHECK(m_lock.try_lock(), ("Cannot create GeoObjectView on locked mutex"));
    }
    boost::optional<base::GeoObjectId> SearchIdOfFirstMatchedObject(
        m2::PointD const & point, std::function<bool(base::GeoObjectId)> && pred) const;

    boost::optional<GeoObjectData> GetGeoData(base::GeoObjectId id) const;

    std::vector<base::GeoObjectId> SearchObjectsInIndex(m2::PointD const & point) const
    {
      return SearchGeoObjectIdsByPoint(m_geoIndex, point);
    }

    base::JSONPtr GetFullGeoObjectWithoutNameAndCoordinates(base::GeoObjectId id) const;

    base::JSONPtr GetFullGeoObject(
        m2::PointD point,
        std::function<bool(GeoObjectMaintainer::GeoObjectData const &)> && pred) const;

    static std::vector<base::GeoObjectId> SearchGeoObjectIdsByPoint(GeoIndex const & index,
                                                                    m2::PointD point);

  private:
    GeoIndex const & m_geoIndex;
    GeoId2GeoData const & m_geoId2GeoData;
    RegionIdGetter const & m_regionIdGetter;
    std::unique_lock<std::mutex> m_lock;
  };

  GeoObjectMaintainer(std::string const & pathOutGeoObjectsKv, RegionInfoGetter && regionInfoGetter,
                      RegionIdGetter && regionIdGetter);

  void SetIndex(GeoIndex && index) { m_index = std::move(index); }

  void StoreAndEnrich(feature::FeatureBuilder & fb);
  void WriteToStorage(base::GeoObjectId id, JsonValue && value);

  size_t Size() const { return m_geoId2GeoData.size(); }

  GeoObjectsView CreateView()
  {
    return GeoObjectsView(m_index, m_geoId2GeoData, m_regionIdGetter, m_updateMutex);
  }

private:
  static std::fstream InitGeoObjectsKv(std::string const & pathOutGeoObjectsKv);

  std::fstream m_geoObjectsKvStorage;
  std::mutex m_updateMutex;
  std::mutex m_storageMutex;

  GeoIndex m_index;
  RegionInfoGetter m_regionInfoGetter;
  RegionIdGetter m_regionIdGetter;
  GeoId2GeoData m_geoId2GeoData;
};
}  // namespace geo_objects
}  // namespace generator
