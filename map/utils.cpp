#include "map/utils.hpp"

#include "map/place_page_info.hpp"

#include "indexer/feature.hpp"
#include "indexer/feature_decl.hpp"
#include "indexer/feature_algo.hpp"

namespace utils
{
eye::MapObject MakeEyeMapObject(place_page::Info const & info)
{
  if (!info.IsFeature())
    return {};

  auto types = info.GetTypes();
  if (types.Empty())
    return {};

  types.SortBySpec();

  eye::MapObject mapObject;
  mapObject.SetBestType(classif().GetReadableObjectName(types.GetBestType()));
  mapObject.SetPos(info.GetMercator());
  mapObject.SetDefaultName(info.GetDefaultName());
  mapObject.SetReadableName(info.GetPrimaryFeatureName());

  return mapObject;
}

eye::MapObject MakeEyeMapObject(FeatureType & ft)
{
  feature::TypesHolder types(ft);
  if (types.Empty())
    return {};

  types.SortBySpec();

  eye::MapObject mapObject;
  mapObject.SetBestType(classif().GetReadableObjectName(types.GetBestType()));
  mapObject.SetPos(feature::GetCenter(ft));

  string name;
  if (ft.GetName(StringUtf8Multilang::kDefaultCode, name))
    mapObject.SetDefaultName(name);

  name.clear();
  ft.GetReadableName(name);
  mapObject.SetReadableName(name);

  return mapObject;
}

search::ReverseGeocoder::Address GetAddressAtPoint(DataSource const & dataSource,
                                                   m2::PointD const & pt)
{
  double const kDistanceThresholdMeters = 0.5;

  search::ReverseGeocoder const coder(dataSource);
  search::ReverseGeocoder::Address address;
  coder.GetNearbyAddress(pt, address);

  // We do not init nearby address info for points that are located
  // outside of the nearby building.
  if (address.GetDistance() < kDistanceThresholdMeters)
    return address;

  return {};
}
}  // namespace utils
