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
class RegionInfoGetterProxy
{
public:
  using RegionInfoGetter = std::function<boost::optional<KeyValue>(m2::PointD const & pathPoint)>;
  using RegionIdGetter = std::function<std::shared_ptr<JsonValue>(base::GeoObjectId id)>;

  RegionInfoGetterProxy(std::string const & pathInRegionsIndex, std::string const & pathInRegionsKv)
  {
    m_regionInfoGetter = regions::RegionInfoGetter(pathInRegionsIndex, pathInRegionsKv);
    LOG(LINFO, ("Size of regions key-value storage:", m_regionInfoGetter->GetStorage().Size()));
  }

  explicit RegionInfoGetterProxy(RegionInfoGetter && regionInfoGetter, RegionIdGetter && regionIdGetter)
    : m_externalInfoGetter(std::move(regionInfoGetter))
    , m_externalRegionIdGetter(std::move(regionIdGetter))

  {
    LOG(LINFO, ("External regions info provided"));
  }

  boost::optional<KeyValue> FindDeepest(m2::PointD const & point) const
  {
    return m_regionInfoGetter ? m_regionInfoGetter->FindDeepest(point)
                              : m_externalInfoGetter->operator()(point);
  }

  std::shared_ptr<JsonValue> FindById(base::GeoObjectId id) const
  {
    return m_externalRegionIdGetter ? m_externalRegionIdGetter->operator()(id) :
           m_regionInfoGetter->GetStorage().Find(id.GetEncodedId());

  }


private:
  boost::optional<regions::RegionInfoGetter> m_regionInfoGetter;
  boost::optional<RegionInfoGetter> m_externalInfoGetter;
  boost::optional<RegionIdGetter> m_externalRegionIdGetter;
};

void UpdateCoordinates(m2::PointD const & point, base::JSONPtr & json);

class GeoObjectMaintainer
{
public:
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
                   RegionInfoGetterProxy const & regionInfoGetter, std::mutex & updateMutex)
      : m_geoIndex(geoIndex)
      , m_geoId2GeoData(geoId2GeoData)
      , m_regionInfoGetter(regionInfoGetter)
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
    RegionInfoGetterProxy const & m_regionInfoGetter;
    std::unique_lock<std::mutex> m_lock;
  };

  GeoObjectMaintainer(std::string const & pathOutGeoObjectsKv,
                      RegionInfoGetterProxy && regionInfoGetter);

  void SetIndex(GeoIndex && index) { m_index = std::move(index); }

  void StoreAndEnrich(feature::FeatureBuilder & fb);
  void WriteToStorage(base::GeoObjectId id, JsonValue && value);

  size_t Size() const { return m_geoId2GeoData.size(); }

  const GeoObjectsView CreateView()
  {
    return GeoObjectsView(m_index, m_geoId2GeoData, m_regionInfoGetter, m_updateMutex);
  }

private:
  static std::fstream InitGeoObjectsKv(std::string const & pathOutGeoObjectsKv);

  std::fstream m_geoObjectsKvStorage;
  std::mutex m_updateMutex;
  std::mutex m_storageMutex;

  GeoIndex m_index;
  RegionInfoGetterProxy m_regionInfoGetter;
  GeoId2GeoData m_geoId2GeoData;
};
}  // namespace geo_objects
}  // namespace generator
