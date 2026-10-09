#include "MapObject.hpp"

#include "app/organicmaps/sdk/bookmarks/data/ApiData.hpp"
#include "app/organicmaps/sdk/bookmarks/data/Bookmark.hpp"
#include "app/organicmaps/sdk/bookmarks/data/Metadata.hpp"
#include "app/organicmaps/sdk/bookmarks/data/OpeningMode.hpp"
#include "app/organicmaps/sdk/bookmarks/data/OsmDescription.hpp"
#include "app/organicmaps/sdk/bookmarks/data/RawData.hpp"
#include "app/organicmaps/sdk/bookmarks/data/RoadWarningMarkType.hpp"
#include "app/organicmaps/sdk/bookmarks/data/Track.hpp"
#include "app/organicmaps/sdk/bookmarks/data/WikiData.hpp"
#include "app/organicmaps/sdk/core/jni_helper.hpp"
#include "app/organicmaps/sdk/routing/RoutingJni.hpp"

#include "map/elevation_info.hpp"
#include "map/place_page_info.hpp"

#include "base/string_utils.hpp"

namespace
{
// TODO(yunikkk): PP can be POI and bookmark at the same time. And can be even POI + bookmark + API at the same time.
// The same for search result: it can be also a POI and bookmark (and API!).
// That is one of the reasons why existing solution should be refactored.
// should be equal with definitions in MapObject.java
static constexpr int kPoi = 0;
static constexpr int kApiPoint = 1;
static constexpr int kBookmark = 2;
static constexpr int kMyPosition = 3;
static constexpr int kSearch = 4;
static constexpr int kTrack = 5;

jobject CreateMapObject(JNIEnv * env, place_page::Info const & info, int mapObjectType, double lat, double lon)
{
  // clang-format off
  static jmethodID const ctorId = jni::GetConstructorID(env, g_mapObjectClazz,
    "("
    "I"                                               // mapObjectType
    "Ljava/lang/String;"                              // title
    "Ljava/lang/String;"                              // secondaryTitle
    "Ljava/lang/String;"                              // subtitle
    "Ljava/lang/String;"                              // address
    "DD"                                              // lat, lon
    ")V"
  );
  // clang-format on

  // clang-format off
  jobject mapObject = env->NewObject(g_mapObjectClazz, ctorId,
    mapObjectType,
    jni::ToJavaStringWithSupplementalCharsFix(env, info.GetTitle()),
    jni::ToJavaStringWithSupplementalCharsFix(env, info.GetSecondaryTitle()),
    jni::ToJavaStringWithSupplementalCharsFix(env, info.GetSubtitle()),
    jni::ToJavaStringWithSupplementalCharsFix(env, info.GetSecondarySubtitle()),
    lat,
    lon
  );
  // clang-format on
  return mapObject;
}

void MapObject_put(JNIEnv * env, jobject mapObject, jobject mapObjectData)
{
  static jmethodID putMethodId =
      env->GetMethodID(g_mapObjectClazz, "put", "(Lapp/organicmaps/sdk/bookmarks/data/MapObjectData;)V");
  ASSERT(putMethodId, ("MapObject.put method not found"));

  env->CallVoidMethod(mapObject, putMethodId, mapObjectData);
}
}  // namespace

jobject CreateMapObject(JNIEnv * env, place_page::Info const & info)
{
  ms::LatLon const ll = info.GetLatLon();
  jobject mapObject = nullptr;

  if (info.IsBookmark())
    mapObject = CreateBookmark(env, info);
  // TODO(yunikkk): object can be POI + API + search result + bookmark simultaneously.
  // TODO(yunikkk): Should we pass localized strings here and in other methods as byte arrays?
  else if (info.IsMyPosition())
  {
    mapObject = CreateMapObject(env, info, kMyPosition, ll.m_lat, ll.m_lon);
  }
  // Classify as an API point when the mark carries a return point id and/or a back URL.
  // (A bare API mark with neither has nothing to surface and falls through to POI below.)
  else if (info.HasApiUrl() || info.HasApiId())
  {
    mapObject = CreateMapObject(env, info, kApiPoint, ll.m_lat, ll.m_lon);
    MapObject_put(env, mapObject, CreateApiData(env, info.GetApiId(), info.GetApiUrl()));
  }
  else if (info.IsTrack())
    mapObject = CreateTrack(env, info);
  else
    mapObject = CreateMapObject(env, info, kPoi, ll.m_lat, ll.m_lon);

  if (info.HasMetadata())
    MapObject_put(env, mapObject, CreateMetadata(env, info));

  if (std::string const & article = info.GetWikiDescription(); !article.empty())
    MapObject_put(env, mapObject, CreateWikiData(env, article));

  if (std::string const & description = info.GetOSMDescription(); !description.empty())
    MapObject_put(env, mapObject, CreateOsmDescription(env, description));

  if (info.IsRoutePoint())
    MapObject_put(env, mapObject, routing_jni::CreateRoutePointInfo(env, info));

  if (auto const rawTypes = info.GetRawTypes(); !rawTypes.empty())
    MapObject_put(env, mapObject, CreateRawData(env, rawTypes));

  if (info.IsRoadType())
    MapObject_put(env, mapObject, CreateRoadWarningMarkType(env, info.GetRoadType()));

  MapObject_put(env, mapObject, CreateOpeningMode(env, info.GetOpeningMode()));

  return mapObject;
}
