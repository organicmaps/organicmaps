#include "UserMarkHelper.hpp"

#include "map/elevation_info.hpp"
#include "map/place_page_info.hpp"

#include "base/string_utils.hpp"

namespace usermark_helper
{
using feature::Metadata;

void InjectMetadata(JNIEnv * env, jclass const clazz, jobject const mapObject, Metadata const & metadata)
{
  static jmethodID const addId = env->GetMethodID(clazz, "addMetadata", "(ILjava/lang/String;)V");
  ASSERT(addId, ());

  for (auto const t : metadata.GetPresentTypes())
  {
    // TODO: It is not a good idea to pass raw strings to UI. Calling separate getters should be a better way.

    std::string meta;
    switch (t)
    {
    case Metadata::FMD_WIKIPEDIA: meta = metadata.GetWikiURL(); break;
    case Metadata::FMD_DESCRIPTION: break;
    default: meta = metadata.Get(static_cast<Metadata::EType>(t)); break;
    }

    if (!meta.empty())
    {
      jni::TScopedLocalRef metaString(env, jni::ToJavaString(env, meta));
      env->CallVoidMethod(mapObject, addId, t, metaString.get());
    }
  }
}

jobject CreatePopularity(JNIEnv * env, place_page::Info const & info)
{
  static jclass const popularityClass =
    jni::GetGlobalClassRef(env, "com/mapswithme/maps/search/Popularity");
  static jmethodID const popularityConstructor =
    jni::GetConstructorID(env, popularityClass, "(I)V");
  auto const popularityValue = info.GetPopularity();
  return env->NewObject(popularityClass, popularityConstructor, static_cast<jint>(popularityValue));
}

jobject CreateMapObject(JNIEnv * env, std::string const & mwmName, int64_t mwmVersion,
                        uint32_t featureIndex, int mapObjectType, std::string const & title,
                        std::string const & secondaryTitle, std::string const & subtitle, double lat,
                        double lon, std::string const & address, Metadata const & metadata,
                        std::string const & apiId,
                        jobject const & routingPointInfo, place_page::OpeningMode openingMode,
                        jobject const & popularity, std::string const & description,
                        RoadWarningMarkType roadWarningMarkType, jobjectArray jrawTypes)
{
  //  public MapObject(@NonNull FeatureId featureId, @MapObjectType int mapObjectType, String title,
  //                   @Nullable String secondaryTitle, String subtitle, String address,
  //                   double lat, double lon, String apiId, @Nullable RoutePointInfo routePointInfo,
  //                   @OpeningMode int openingMode, @NonNull Popularity popularity, @NonNull String description,
  //                   int roadWarningType, @Nullable String[] rawTypes)
  static jmethodID const ctorId = jni::GetConstructorID(
      env, g_mapObjectClazz,
      "("
      "Lcom/mapswithme/maps/bookmarks/data/FeatureId;"  // featureId
      "I"                                               // mapObjectType
      "Ljava/lang/String;"                              // title
      "Ljava/lang/String;"                              // secondaryTitle
      "Ljava/lang/String;"                              // subtitle
      "Ljava/lang/String;"                              // address
      "DD"                                              // lat, lon
      "Ljava/lang/String;"                              // appId
      "Lcom/mapswithme/maps/routing/RoutePointInfo;"    // routePointInfo
      "I"                                               // openingMode
      "Lcom/mapswithme/maps/search/Popularity;"         // popularity
      "Ljava/lang/String;"                              // description
      "I"                                               // roadWarnType
      "[Ljava/lang/String;"                             // rawTypes
      ")V");

  //public FeatureId(@NonNull String mwmName, long mwmVersion, int featureIndex)
  static jmethodID const featureCtorId =
      jni::GetConstructorID(env, g_featureIdClazz, "(Ljava/lang/String;JI)V");

  jni::TScopedLocalRef jMwmName(env, jni::ToJavaString(env, mwmName));
  jni::TScopedLocalRef jFeatureId(
      env, env->NewObject(g_featureIdClazz, featureCtorId, jMwmName.get(), (jlong)mwmVersion,
                          (jint)featureIndex));
  jni::TScopedLocalRef jTitle(env, jni::ToJavaString(env, title));
  jni::TScopedLocalRef jSecondaryTitle(env, jni::ToJavaString(env, secondaryTitle));
  jni::TScopedLocalRef jSubtitle(env, jni::ToJavaString(env, subtitle));
  jni::TScopedLocalRef jAddress(env, jni::ToJavaString(env, address));
  jni::TScopedLocalRef jApiId(env, jni::ToJavaString(env, apiId));
  jni::TScopedLocalRef jDescription(env, jni::ToJavaString(env, description));
  jobject mapObject =
      env->NewObject(g_mapObjectClazz, ctorId, jFeatureId.get(), mapObjectType, jTitle.get(),
                     jSecondaryTitle.get(), jSubtitle.get(), jAddress.get(), lat, lon, jApiId.get(),
                     routingPointInfo,
                     static_cast<jint>(openingMode), popularity, jDescription.get(),
                     static_cast<jint>(roadWarningMarkType), jrawTypes);

  InjectMetadata(env, g_mapObjectClazz, mapObject, metadata);
  return mapObject;
}

jobject CreateBookmark(JNIEnv *env, const place_page::Info &info,
                       const jni::TScopedLocalObjectArrayRef &jrawTypes,
                       const jni::TScopedLocalRef &routingPointInfo,
                       const jni::TScopedLocalRef &popularity)
{
  //public Bookmark(@NonNull FeatureId featureId, @IntRange(from = 0) long categoryId,
  //                @IntRange(from = 0) long bookmarkId, String title, @Nullable String secondaryTitle,
  //                @Nullable String subtitle, @Nullable String address, @Nullable RoutePointInfo routePointInfo,
  //                @OpeningMode int openingMode, @NonNull Popularity popularity, @NonNull String description,
  //                @Nullable String[] rawTypes)
  static jmethodID const ctorId =
          jni::GetConstructorID(env, g_bookmarkClazz,
                                "(Lcom/mapswithme/maps/bookmarks/data/FeatureId;JJLjava/lang/String;"
                                "Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;"
                                "Lcom/mapswithme/maps/routing/RoutePointInfo;"
                                "ILcom/mapswithme/maps/search/Popularity;Ljava/lang/String;"
                                "[Ljava/lang/String;)V");
  static jmethodID const featureCtorId =
          jni::GetConstructorID(env, g_featureIdClazz, "(Ljava/lang/String;JI)V");

  auto const bookmarkId = info.GetBookmarkId();
  auto const categoryId = info.GetBookmarkCategoryId();
  jni::TScopedLocalRef jMwmName(env, jni::ToJavaString(env, info.GetID().GetMwmName()));
  jni::TScopedLocalRef jFeatureId(
          env, env->NewObject(g_featureIdClazz, featureCtorId, jMwmName.get(),
                              (jlong)info.GetID().GetMwmVersion(), (jint)info.GetID().m_index));
  jni::TScopedLocalRef jTitle(env, jni::ToJavaString(env, info.GetTitle()));
  jni::TScopedLocalRef jSecondaryTitle(env, jni::ToJavaString(env, info.GetSecondaryTitle()));
  jni::TScopedLocalRef jSubtitle(env, jni::ToJavaString(env, info.GetSubtitle()));
  jni::TScopedLocalRef jAddress(env, jni::ToJavaString(env, info.GetAddress()));
  jni::TScopedLocalRef jDescription(env, jni::ToJavaString(env, info.GetDescription()));
  jobject mapObject = env->NewObject(
          g_bookmarkClazz, ctorId, jFeatureId.get(), static_cast<jlong>(categoryId),
          static_cast<jlong>(bookmarkId), jTitle.get(), jSecondaryTitle.get(), jSubtitle.get(),
          jAddress.get(), routingPointInfo.get(), info.GetOpeningMode(), popularity.get(),
          jDescription.get(), jrawTypes.get());

  if (info.HasMetadata())
    InjectMetadata(env, g_mapObjectClazz, mapObject, info.GetMetadata());
  return mapObject;
}

jobject CreateElevationPoint(JNIEnv * env, ElevationInfo::Point const & point)
{
  static jclass const pointClass =
      jni::GetGlobalClassRef(env, "com/mapswithme/maps/bookmarks/data/ElevationInfo$Point");
  // public Point(double distance, int altitude)
  static jmethodID const pointCtorId =
      jni::GetConstructorID(env, pointClass, "(DI)V");
  return env->NewObject(pointClass, pointCtorId, static_cast<jdouble >(point.m_distance),
                        static_cast<jint>(point.m_altitude));
}

jobjectArray ToElevationPointArray(JNIEnv * env, ElevationInfo::Points const & points)
{
  CHECK(!points.empty(), ("Elevation points must be non empty!"));
  static jclass const pointClass =
      jni::GetGlobalClassRef(env, "com/mapswithme/maps/bookmarks/data/ElevationInfo$Point");
  return jni::ToJavaArray(env, pointClass, points,
                         [](JNIEnv * env, ElevationInfo::Point const & item)
                         {
                           return CreateElevationPoint(env, item);
                         });
}

jobject CreateElevationInfo(JNIEnv * env, ElevationInfo const & info)
{
  // public ElevationInfo(long trackId, @NonNull String name, @NonNull Point[] points,
  //                      int ascent, int descent, int minAltitude, int maxAltitude, int difficulty,
  //                      long m_duration)
  static jmethodID const ctorId =
      jni::GetConstructorID(env, g_elevationInfoClazz, "(JLjava/lang/String;Ljava/lang/String;"
                                                       "[Lcom/mapswithme/maps/bookmarks/data/ElevationInfo$Point;"
                                                       "IIIIIJ)V");
  jni::TScopedLocalRef jName(env, jni::ToJavaString(env, info.GetName()));
  jni::TScopedLocalObjectArrayRef jPoints(env, ToElevationPointArray(env, info.GetPoints()));
  return env->NewObject(g_elevationInfoClazz, ctorId, static_cast<jlong>(info.GetId()),
                        jName.get(), jPoints.get(),
                        static_cast<jint>(info.GetAscent()),
                        static_cast<jint>(info.GetDescent()),
                        static_cast<jint>(info.GetMinAltitude()),
                        static_cast<jint>(info.GetMaxAltitude()),
                        static_cast<jint>(info.GetDifficulty()),
                        static_cast<jlong>(info.GetDuration()));
}

jobject CreateMapObject(JNIEnv * env, place_page::Info const & info)
{
  jni::TScopedLocalObjectArrayRef jrawTypes(env, jni::ToJavaStringArray(env, info.GetRawTypes()));

  jni::TScopedLocalRef routingPointInfo(env, nullptr);
  if (info.IsRoutePoint())
    routingPointInfo.reset(CreateRoutePointInfo(env, info));

  jni::TScopedLocalRef popularity(env, CreatePopularity(env, info));

  if (info.IsBookmark())
  {
    return CreateBookmark(env, info, jrawTypes, routingPointInfo, popularity);
  }

  ms::LatLon const ll = info.GetLatLon();
  // TODO(yunikkk): object can be POI + API + search result + bookmark simultaneously.
  // TODO(yunikkk): Should we pass localized strings here and in other methods as byte arrays?
  if (info.IsMyPosition())
  {
    return CreateMapObject(env, info.GetID().GetMwmName(), info.GetID().GetMwmVersion(),
                           info.GetID().m_index, kMyPosition, info.GetTitle(),
                           info.GetSecondaryTitle(), info.GetSubtitle(), ll.m_lat, ll.m_lon,
                           info.GetAddress(), {}, "",
                           routingPointInfo.get(), info.GetOpeningMode(),
                           popularity.get(), info.GetDescription(), info.GetRoadType(),
                           jrawTypes.get());
  }

  if (info.HasApiUrl())
  {
    return CreateMapObject(
        env, info.GetID().GetMwmName(), info.GetID().GetMwmVersion(), info.GetID().m_index,
        kApiPoint, info.GetTitle(), info.GetSecondaryTitle(), info.GetSubtitle(), ll.m_lat, ll.m_lon,
        info.GetAddress(), info.GetMetadata(), info.GetApiUrl(),
        routingPointInfo.get(), info.GetOpeningMode(), popularity.get(), info.GetDescription(),
        info.GetRoadType(), jrawTypes.get());
  }

  return CreateMapObject(
      env, info.GetID().GetMwmName(), info.GetID().GetMwmVersion(), info.GetID().m_index, kPoi,
      info.GetTitle(), info.GetSecondaryTitle(), info.GetSubtitle(), ll.m_lat, ll.m_lon,
      info.GetAddress(), info.HasMetadata() ? info.GetMetadata() : Metadata(), "",
      routingPointInfo.get(), info.GetOpeningMode(), popularity.get(), info.GetDescription(),
      info.GetRoadType(), jrawTypes.get());
}

jobject CreateRoutePointInfo(JNIEnv * env, place_page::Info const & info)
{
  static jclass const clazz = jni::GetGlobalClassRef(env, "com/mapswithme/maps/routing/RoutePointInfo");
  static jmethodID const ctorId = jni::GetConstructorID(env, clazz, "(II)V");
  int const markType = static_cast<int>(info.GetRouteMarkType());
  return env->NewObject(clazz, ctorId, markType, info.GetIntermediateIndex());
}

jobject CreateFeatureId(JNIEnv * env, FeatureID const & fid)
{
  static jmethodID const featureCtorId =
    jni::GetConstructorID(env, g_featureIdClazz, "(Ljava/lang/String;JI)V");

  auto const & info = fid.m_mwmId.GetInfo();
  jni::TScopedLocalRef jMwmName(env, jni::ToJavaString(env, info ? info->GetCountryName() : ""));
  return env->NewObject(g_featureIdClazz, featureCtorId, jMwmName.get(),
                        info ? static_cast<jlong>(info->GetVersion()) : 0,
                        static_cast<jint>(fid.m_index));
}

jobjectArray ToFeatureIdArray(JNIEnv * env, std::vector<FeatureID> const & ids)
{
  if (ids.empty())
    return nullptr;

  return jni::ToJavaArray(env, g_featureIdClazz, ids,
                          [](JNIEnv * env, FeatureID const & fid) {
                            return CreateFeatureId(env, fid);
                          });
}
}  // namespace usermark_helper
