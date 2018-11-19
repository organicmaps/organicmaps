#include "map/utils.hpp"

#include "map/place_page_info.hpp"

#include "indexer/feature.hpp"
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
}  // namespace utils
