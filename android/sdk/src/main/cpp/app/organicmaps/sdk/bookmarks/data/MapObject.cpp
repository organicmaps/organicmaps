#include "MapObject.hpp"

#include "app/organicmaps/sdk/bookmarks/data/Bookmark.hpp"
#include "app/organicmaps/sdk/bookmarks/data/Metadata.hpp"
#include "app/organicmaps/sdk/bookmarks/data/Track.hpp"
#include "app/organicmaps/sdk/core/jni_helper.hpp"
#include "app/organicmaps/sdk/routing/RoutePointInfo.hpp"

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
}  // namespace

jobject CreateMapObject(JNIEnv * env, place_page::Info const & info, int mapObjectType, double lat, double lon,
                        bool parseMeta, bool parseApi, jobject const & routingPointInfo, jobjectArray jrawTypes)
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
    "Ljava/lang/String;"                              // appId
    "Lapp/organicmaps/sdk/routing/RoutePointInfo;"    // routePointInfo
    "I"                                               // openingMode
    "Ljava/lang/String;"                              // wikiArticle
    "Ljava/lang/String;"                              // osmDescription
    "I"                                               // roadWarnType
    "[Ljava/lang/String;"                             // rawTypes
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
    lon,
    jni::ToJavaString(env, parseApi ? info.GetApiUrl() : ""),
    routingPointInfo,
    static_cast<jint>(info.GetOpeningMode()),
    jni::ToJavaString(env, info.GetWikiDescription()),
    jni::ToJavaString(env, info.GetOSMDescription()),
    static_cast<jint>(info.GetRoadType()),
    jrawTypes
  );
  // clang-format on

  if (parseMeta)
    InjectMetadata(env, g_mapObjectClazz, mapObject, info);
  return mapObject;
}

jobject CreateMapObject(JNIEnv * env, place_page::Info const & info)
{
  jni::TScopedLocalObjectArrayRef jrawTypes(env, jni::ToJavaStringArray(env, info.GetRawTypes()));

  jni::TScopedLocalRef routingPointInfo(env, nullptr);
  if (info.IsRoutePoint())
    routingPointInfo.reset(CreateRoutePointInfo(env, info));

  if (info.IsBookmark())
    return CreateBookmark(env, info, jrawTypes, routingPointInfo);

  ms::LatLon const ll = info.GetLatLon();
  // TODO(yunikkk): object can be POI + API + search result + bookmark simultaneously.
  // TODO(yunikkk): Should we pass localized strings here and in other methods as byte arrays?
  if (info.IsMyPosition())
  {
    return CreateMapObject(env, info, kMyPosition, ll.m_lat, ll.m_lon, false /* parseMeta */, false /* parseApi */,
                           routingPointInfo.get(), jrawTypes.get());
  }

  if (info.HasApiUrl())
  {
    return CreateMapObject(env, info, kApiPoint, ll.m_lat, ll.m_lon, true /* parseMeta */, true /* parseApi */,
                           routingPointInfo.get(), jrawTypes.get());
  }

  if (info.IsTrack())
    return CreateTrack(env, info, jrawTypes, routingPointInfo);

  return CreateMapObject(env, info, kPoi, ll.m_lat, ll.m_lon, true /* parseMeta */, false /* parseApi */,
                         routingPointInfo.get(), jrawTypes.get());
}
