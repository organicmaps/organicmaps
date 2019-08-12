#include "generator/geo_objects/geo_objects_maintainer.hpp"
#include "generator/geo_objects/geo_objects_filter.hpp"

#include "generator/translation.hpp"

#include <utility>

namespace generator
{
namespace geo_objects
{
GeoObjectMaintainer::GeoObjectMaintainer(std::string const & pathOutGeoObjectsKv,
                                         RegionInfoGetterProxy && regionInfoGetter)
  : m_geoObjectsKvStorage{InitGeoObjectsKv(pathOutGeoObjectsKv)}
  , m_regionInfoGetter{std::move(regionInfoGetter)}
{
}

// static
std::fstream GeoObjectMaintainer::InitGeoObjectsKv(std::string const & pathOutGeoObjectsKv)
{
  std::fstream result{pathOutGeoObjectsKv,
                      std::ios_base::in | std::ios_base::out | std::ios_base::app};
  if (!result)
    MYTHROW(Reader::OpenException, ("Failed to open file", pathOutGeoObjectsKv));

  return std::move(result);
}

void UpdateCoordinates(m2::PointD const & point, base::JSONPtr & json)
{
  auto geometry = json_object_get(json.get(), "geometry");
  auto coordinates = json_object_get(geometry, "coordinates");
  if (json_array_size(coordinates) == 2)
  {
    auto const latLon = MercatorBounds::ToLatLon(point);
    json_array_set_new(coordinates, 0, ToJSON(latLon.m_lon).release());
    json_array_set_new(coordinates, 1, ToJSON(latLon.m_lat).release());
  }
}

base::JSONPtr AddAddress(std::string const & street, std::string const & house, m2::PointD point,
                         StringUtf8Multilang const & name, KeyValue const & regionKeyValue)
{
  auto result = regionKeyValue.second->MakeDeepCopyJson();

  UpdateCoordinates(point, result);

  auto properties = base::GetJSONObligatoryField(result.get(), "properties");
  auto address = base::GetJSONObligatoryFieldByPath(properties, "locales", "default", "address");
  if (!street.empty())
    ToJSONObject(*address, "street", street);

  // By writing home null in the field we can understand that the house has no address.
  if (!house.empty())
    ToJSONObject(*address, "building", house);
  else
    ToJSONObject(*address, "building", base::NewJSONNull());

  Localizator localizator(*properties);
  localizator.SetLocale("name", Localizator::EasyObjectWithTranslation(name));

  int const kHouseOrPoiRank = 30;
  ToJSONObject(*properties, "rank", kHouseOrPoiRank);

  ToJSONObject(*properties, "dref", KeyValueStorage::SerializeDref(regionKeyValue.first));
  // auto locales = json_object_get(result.get(), "locales");
  // auto locales = json_object_get(result.get(), "locales");
  // auto en = json_object_get(result.get(), "en");
  // todo(maksimandrianov): Add en locales.
  return result;
}

void GeoObjectMaintainer::StoreAndEnrich(feature::FeatureBuilder & fb)
{
  if (!GeoObjectsFilter::IsBuilding(fb) && !GeoObjectsFilter::HasHouse(fb))
    return;

  auto regionKeyValue = m_regionInfoGetter.FindDeepest(fb.GetKeyPoint());
  if (!regionKeyValue)
    return;

  auto const id = fb.GetMostGenericOsmId();
  auto jsonValue = AddAddress(fb.GetParams().GetStreet(), fb.GetParams().house.Get(),
                              fb.GetKeyPoint(), fb.GetMultilangName(), *regionKeyValue);

  std::lock_guard<std::mutex> lock(m_updateMutex);

  auto const it = m_geoId2GeoData.emplace(
      std::make_pair(id, GeoObjectData{fb.GetParams().GetStreet(), fb.GetParams().house.Get(),
                                       fb.GetKeyPoint(), fb.GetMultilangName()}));

  // Duplicate ID's are possible
  if (!it.second)
    return;

  m_geoObjectsKvStorage << KeyValueStorage::SerializeFullLine(id.GetEncodedId(),
                                                              JsonValue{std::move(jsonValue)});
}

void GeoObjectMaintainer::WriteToStorage(base::GeoObjectId id, JsonValue && value)
{
  std::lock_guard<std::mutex> lock(m_updateMutex);
  m_geoObjectsKvStorage << KeyValueStorage::SerializeFullLine(id.GetEncodedId(), std::move(value));
}

// GeoObjectMaintainer::GeoObjectsView

base::JSONPtr GeoObjectMaintainer::GeoObjectsView::GetFullGeoObject(
    m2::PointD point,
    std::function<bool(GeoObjectMaintainer::GeoObjectData const &)> && pred) const
{
  auto const ids = SearchGeoObjectIdsByPoint(m_geoIndex, point);
  for (auto const & id : ids)
  {
    auto const it = m_geoId2GeoData.find(id);
    if (it == m_geoId2GeoData.end())
      continue;

    auto const geoData = it->second;
    if (!pred(geoData))
      continue;

    auto regionKeyValue = m_regionInfoGetter.FindDeepest(geoData.m_keyPoint);
    if (!regionKeyValue)
      return {};

    return AddAddress(geoData.m_street, geoData.m_house, geoData.m_keyPoint, geoData.m_name,
                      *regionKeyValue);
  }

  return {};
}

base::JSONPtr GeoObjectMaintainer::GeoObjectsView::GetFullGeoObject(base::GeoObjectId id) const
{
  auto const it = m_geoId2GeoData.find(id);
  if (it == m_geoId2GeoData.end())
    return {};

  auto const geoData = it->second;
  auto regionKeyValue = m_regionInfoGetter.FindDeepest(geoData.m_keyPoint);
  if (!regionKeyValue)
    return {};

  return AddAddress(geoData.m_street, geoData.m_house, geoData.m_keyPoint, geoData.m_name,
                    *regionKeyValue);
}

boost::optional<GeoObjectMaintainer::GeoObjectData> GeoObjectMaintainer::GeoObjectsView::GetGeoData(
    base::GeoObjectId id) const
{
  auto const it = m_geoId2GeoData.find(id);
  if (it == m_geoId2GeoData.end())
    return {};

  return it->second;
}

boost::optional<base::GeoObjectId>
GeoObjectMaintainer::GeoObjectsView::SearchIdOfFirstMatchedObject(
    m2::PointD const & point, std::function<bool(base::GeoObjectId)> && pred) const
{
  auto const ids = SearchObjectsInIndex(point);
  for (auto const & id : ids)
  {
    if (pred(id))
      return id;
  }

  return {};
}

std::vector<base::GeoObjectId> GeoObjectMaintainer::GeoObjectsView::SearchGeoObjectIdsByPoint(
    GeoIndex const & index, m2::PointD point)
{
  std::vector<base::GeoObjectId> ids;
  auto const emplace = [&ids](base::GeoObjectId const & osmId) { ids.emplace_back(osmId); };
  index.ForEachAtPoint(emplace, point);
  return ids;
}
}  // namespace geo_objects
}  // namespace generator
