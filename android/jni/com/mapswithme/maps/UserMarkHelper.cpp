#include "UserMarkHelper.hpp"

#include "map/elevation_info.hpp"
#include "map/place_page_info.hpp"

#include "partners_api/ads/ads_engine.hpp"
#include "partners_api/ads/banner.hpp"

#include "base/string_utils.hpp"

namespace usermark_helper
{
using feature::Metadata;

void InjectMetadata(JNIEnv * env, jclass const clazz, jobject const mapObject, feature::Metadata const & metadata)
{
  static jmethodID const addId = env->GetMethodID(clazz, "addMetadata", "(ILjava/lang/String;)V");
  ASSERT(addId, ());

  for (auto const t : metadata.GetPresentTypes())
  {
    // TODO: It is not a good idea to pass raw strings to UI. Calling separate getters should be a better way.
    jni::TScopedLocalRef metaString(env, t == feature::Metadata::FMD_WIKIPEDIA ?
                                              jni::ToJavaString(env, metadata.GetWikiURL()) :
                                              jni::ToJavaString(env, metadata.Get(t)));
    env->CallVoidMethod(mapObject, addId, t, metaString.get());
  }
}

jobject CreateBanner(JNIEnv * env, std::string const & id, jint type)
{
  static jmethodID const bannerCtorId =
      jni::GetConstructorID(env, g_bannerClazz, "(Ljava/lang/String;I)V");
  jni::TScopedLocalRef idRef(env, jni::ToJavaString(env, id));
  return env->NewObject(g_bannerClazz, bannerCtorId, idRef.get(), type);
}

jobject CreateRating(JNIEnv * env, std::string const & name)
{
  static jmethodID const ratingCtorId =
      jni::GetConstructorID(env, g_ratingClazz, "(Ljava/lang/String;F)V");
  jni::TScopedLocalRef nameRef(env, jni::ToJavaString(env, name));
  return env->NewObject(g_ratingClazz, ratingCtorId, nameRef.get(), place_page::kIncorrectRating);
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

jobject CreateHotelType(JNIEnv * env, place_page::Info const & info)
{
  if (!info.GetHotelType())
    return nullptr;

  static jclass const hotelTypeClass =
    jni::GetGlobalClassRef(env, "com/mapswithme/maps/search/HotelsFilter$HotelType");
  static jmethodID const hotelTypeCtorId =
    jni::GetConstructorID(env, hotelTypeClass, "(ILjava/lang/String;)V");

  auto const tag = ftypes::IsHotelChecker::GetHotelTypeTag(*info.GetHotelType());
  return env->NewObject(hotelTypeClass, hotelTypeCtorId, static_cast<jint>(*info.GetHotelType()),
                        jni::ToJavaString(env, tag));
}

jobject CreateMapObject(JNIEnv * env, std::string const & mwmName, int64_t mwmVersion,
                        uint32_t featureIndex, int mapObjectType, std::string const & title,
                        std::string const & secondaryTitle, std::string const & subtitle, double lat,
                        double lon, std::string const & address, Metadata const & metadata,
                        std::string const & apiId, jobjectArray jbanners, jintArray jTaxiTypes,
                        std::string const & bookingSearchUrl, jobject const & localAdInfo,
                        jobject const & routingPointInfo, place_page::OpeningMode openingMode,
                        bool shouldShowUGC, bool canBeRated, bool canBeReviewed,
                        jobjectArray jratings, jobject const & hotelType, int priceRate,
                        jobject const & popularity, std::string const & description,
                        RoadWarningMarkType roadWarningMarkType, bool isTopChoice,
                        jobjectArray jrawTypes)
{
  // public MapObject(@NonNull FeatureId featureId, @MapObjectType int mapObjectType, String title,
  //                  @Nullable String secondaryTitle, String subtitle, String address,
  //                  double lat, double lon, String apiId, @Nullable Banner[] banners,
  //                  @Nullable int[] types, @Nullable String bookingSearchUrl,
  //                  @Nullable LocalAdInfo localAdInfo, @Nullable RoutePointInfo routePointInfo,
  //                  @OpeningMode int openingMode, boolean shouldShowUGC, boolean canBeRated,
  //                  boolean canBeReviewed, @Nullable UGC.Rating[] ratings,
  //                  @Nullable HotelsFilter.HotelType hotelType,
  //                  @PriceFilterView.PriceDef int priceRate
  //                  boolean isTopChoice)
  static jmethodID const ctorId = jni::GetConstructorID(
      env, g_mapObjectClazz,
      "(Lcom/mapswithme/maps/bookmarks/data/FeatureId;ILjava/lang/String;Ljava/lang/"
      "String;Ljava/lang/String;Ljava/lang/String;DDLjava/lang/"
      "String;[Lcom/mapswithme/maps/ads/Banner;[ILjava/lang/String;"
      "Lcom/mapswithme/maps/ads/LocalAdInfo;"
      "Lcom/mapswithme/maps/routing/RoutePointInfo;IZZZ[Lcom/mapswithme/maps/ugc/UGC$Rating;"
      "Lcom/mapswithme/maps/search/HotelsFilter$HotelType;ILcom/mapswithme/maps/search/Popularity;"
      "Ljava/lang/String;IZ[Ljava/lang/String;)V");

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
  jni::TScopedLocalRef jBookingSearchUrl(env, jni::ToJavaString(env, bookingSearchUrl));
  jni::TScopedLocalRef jDescription(env, jni::ToJavaString(env, description));
  jobject mapObject =
      env->NewObject(g_mapObjectClazz, ctorId, jFeatureId.get(), mapObjectType, jTitle.get(),
                     jSecondaryTitle.get(), jSubtitle.get(), jAddress.get(), lat, lon, jApiId.get(),
                     jbanners, jTaxiTypes, jBookingSearchUrl.get(), localAdInfo, routingPointInfo,
                     static_cast<jint>(openingMode), static_cast<jboolean>(shouldShowUGC),
                     static_cast<jboolean>(canBeRated), static_cast<jboolean>(canBeReviewed),
                     jratings, hotelType, priceRate, popularity, jDescription.get(),
                     static_cast<jint>(roadWarningMarkType), static_cast<jboolean>(isTopChoice),
                     jrawTypes);

  InjectMetadata(env, g_mapObjectClazz, mapObject, metadata);
  return mapObject;
}

jobject CreateBookmark(JNIEnv *env, const place_page::Info &info,
                       const jni::TScopedLocalObjectArrayRef &jbanners,
                       const jni::TScopedLocalObjectArrayRef &jratings,
                       const jni::TScopedLocalIntArrayRef &jTaxiTypes,
                       const jni::TScopedLocalObjectArrayRef &jrawTypes,
                       const jni::TScopedLocalRef &localAdInfo,
                       const jni::TScopedLocalRef &routingPointInfo,
                       const jni::TScopedLocalRef &hotelType,
                       const jni::TScopedLocalRef &popularity, int priceRate)
{
  // public Bookmark(@NonNull FeatureId featureId, @IntRange(from = 0) long categoryId,
  //                 @IntRange(from = 0) long bookmarkId, String title, @Nullable String secondaryTitle,
  //                 @Nullable String subtitle, @Nullable String address, @Nullable Banner[] banners,
  //                 @Nullable int[] reachableByTaxiTypes, @Nullable String bookingSearchUrl,
  //                 @Nullable LocalAdInfo localAdInfo, @Nullable RoutePointInfo routePointInfo,
  //                 @OpeningMode int openingMode, boolean shouldShowUGC, boolean canBeRated,
  //                 boolean canBeReviewed, @Nullable UGC.Rating[] ratings,
  //                 @Nullable HotelsFilter.HotelType hotelType,
  //                 @PriceFilterView.PriceDef int priceRate,
  //                 @NotNull com.mapswithme.maps.search.Popularity entity
  //                 @NotNull String description
  //                 boolean isTopChoice)
  static jmethodID const ctorId =
          jni::GetConstructorID(env, g_bookmarkClazz,
                                "(Lcom/mapswithme/maps/bookmarks/data/FeatureId;JJLjava/lang/String;"
                                "Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;"
                                "[Lcom/mapswithme/maps/ads/Banner;[ILjava/lang/String;"
                                "Lcom/mapswithme/maps/ads/LocalAdInfo;"
                                "Lcom/mapswithme/maps/routing/RoutePointInfo;"
                                "IZZZ[Lcom/mapswithme/maps/ugc/UGC$Rating;"
                                "Lcom/mapswithme/maps/search/HotelsFilter$HotelType;"
                                "ILcom/mapswithme/maps/search/Popularity;Ljava/lang/String;"
                                "Z[Ljava/lang/String;)V");
  // public FeatureId(@NonNull String mwmName, long mwmVersion, int featureIndex)
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
  jni::TScopedLocalRef jBookingSearchUrl(env, jni::ToJavaString(env, info.GetBookingSearchUrl()));
  jobject mapObject = env->NewObject(
          g_bookmarkClazz, ctorId, jFeatureId.get(), static_cast<jlong>(categoryId),
          static_cast<jlong>(bookmarkId), jTitle.get(), jSecondaryTitle.get(), jSubtitle.get(),
          jAddress.get(), jbanners.get(), jTaxiTypes.get(), jBookingSearchUrl.get(),
          localAdInfo.get(), routingPointInfo.get(), info.GetOpeningMode(), info.ShouldShowUGC(),
          info.CanBeRated(), info.CanBeReviewed(), jratings.get(), hotelType.get(), priceRate,
          popularity.get(), jDescription.get(), info.IsTopChoice(), jrawTypes.get());

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

jobject CreateElevationInfo(JNIEnv * env, std::string const & serverId, ElevationInfo const & info)
{
  // public ElevationInfo(long trackId, @NonNull String name, @NonNull Point[] points,
  //                      int ascent, int descent, int minAltitude, int maxAltitude, int difficulty,
  //                      long m_duration)
  static jmethodID const ctorId =
      jni::GetConstructorID(env, g_elevationInfoClazz, "(JLjava/lang/String;Ljava/lang/String;"
                                                       "[Lcom/mapswithme/maps/bookmarks/data/ElevationInfo$Point;"
                                                       "IIIIIJ)V");
  jni::TScopedLocalRef jServerId(env, jni::ToJavaString(env, serverId));
  jni::TScopedLocalRef jName(env, jni::ToJavaString(env, info.GetName()));
  jni::TScopedLocalObjectArrayRef jPoints(env, ToElevationPointArray(env, info.GetPoints()));
  return env->NewObject(g_elevationInfoClazz, ctorId, static_cast<jlong>(info.GetId()),
                        jServerId.get(), jName.get(), jPoints.get(),
                        static_cast<jint>(info.GetAscent()),
                        static_cast<jint>(info.GetDescent()),
                        static_cast<jint>(info.GetMinAltitude()),
                        static_cast<jint>(info.GetMaxAltitude()),
                        static_cast<jint>(info.GetDifficulty()),
                        static_cast<jlong>(info.GetDuration()));
}

jobject CreateMapObject(JNIEnv * env, place_page::Info const & info)
{
  jni::TScopedLocalObjectArrayRef jbanners(env, nullptr);
  auto const banners = info.GetBanners();
  if (!banners.empty())
    jbanners.reset(ToBannersArray(env, banners));

  jni::TScopedLocalObjectArrayRef jratings(env, ToRatingArray(env, info.GetRatingCategories()));

  jni::TScopedLocalIntArrayRef jTaxiTypes(env, ToReachableByTaxiProvidersArray(env, info.ReachableByTaxiProviders()));

  jni::TScopedLocalObjectArrayRef jrawTypes(env, jni::ToJavaStringArray(env, info.GetRawTypes()));

  jni::TScopedLocalRef localAdInfo(env, CreateLocalAdInfo(env, info));

  jni::TScopedLocalRef routingPointInfo(env, nullptr);
  if (info.IsRoutePoint())
    routingPointInfo.reset(CreateRoutePointInfo(env, info));

  jni::TScopedLocalRef hotelType(env, CreateHotelType(env, info));
  jni::TScopedLocalRef popularity(env, CreatePopularity(env, info));

  int priceRate = info.GetRawApproximatePricing().value_or(kPriceRateUndefined);

  if (info.IsBookmark())
  {
    return CreateBookmark(env, info, jbanners, jratings, jTaxiTypes, jrawTypes, localAdInfo,
                          routingPointInfo, hotelType, popularity, priceRate);
  }

  ms::LatLon const ll = info.GetLatLon();
  // TODO(yunikkk): object can be POI + API + search result + bookmark simultaneously.
  // TODO(yunikkk): Should we pass localized strings here and in other methods as byte arrays?
  if (info.IsMyPosition())
  {
    return CreateMapObject(env, info.GetID().GetMwmName(), info.GetID().GetMwmVersion(),
                           info.GetID().m_index, kMyPosition, info.GetTitle(),
                           info.GetSecondaryTitle(), info.GetSubtitle(), ll.m_lat, ll.m_lon,
                           info.GetAddress(), {}, "", jbanners.get(), jTaxiTypes.get(),
                           info.GetBookingSearchUrl(), localAdInfo.get(), routingPointInfo.get(),
                           info.GetOpeningMode(), info.ShouldShowUGC(), info.CanBeRated(),
                           info.CanBeReviewed(), jratings.get(), hotelType.get(), priceRate,
                           popularity.get(), info.GetDescription(), info.GetRoadType(),
                           info.IsTopChoice(), jrawTypes.get());
  }

  if (info.HasApiUrl())
  {
    return CreateMapObject(
        env, info.GetID().GetMwmName(), info.GetID().GetMwmVersion(), info.GetID().m_index,
        kApiPoint, info.GetTitle(), info.GetSecondaryTitle(), info.GetSubtitle(), ll.m_lat, ll.m_lon,
        info.GetAddress(), info.GetMetadata(), info.GetApiUrl(), jbanners.get(), jTaxiTypes.get(),
        info.GetBookingSearchUrl(), localAdInfo.get(), routingPointInfo.get(), info.GetOpeningMode(),
        info.ShouldShowUGC(), info.CanBeRated(), info.CanBeReviewed(), jratings.get(),
        hotelType.get(), priceRate, popularity.get(), info.GetDescription(), info.GetRoadType(),
        info.IsTopChoice(), jrawTypes.get());
  }

  return CreateMapObject(
      env, info.GetID().GetMwmName(), info.GetID().GetMwmVersion(), info.GetID().m_index, kPoi,
      info.GetTitle(), info.GetSecondaryTitle(), info.GetSubtitle(), ll.m_lat, ll.m_lon,
      info.GetAddress(), info.HasMetadata() ? info.GetMetadata() : Metadata(), "", jbanners.get(),
      jTaxiTypes.get(), info.GetBookingSearchUrl(), localAdInfo.get(), routingPointInfo.get(),
      info.GetOpeningMode(), info.ShouldShowUGC(), info.CanBeRated(), info.CanBeReviewed(),
      jratings.get(), hotelType.get(), priceRate, popularity.get(), info.GetDescription(),
      info.GetRoadType(), info.IsTopChoice(), jrawTypes.get());
}

jobjectArray ToBannersArray(JNIEnv * env, std::vector<ads::Banner> const & banners)
{
  return jni::ToJavaArray(env, g_bannerClazz, banners,
                          [](JNIEnv * env, ads::Banner const & item) {
                            return CreateBanner(env, item.m_value, static_cast<jint>(item.m_type));
                          });
}

jobjectArray ToRatingArray(JNIEnv * env, std::vector<std::string> const & ratingCategories)
{
  if (ratingCategories.empty())
    return nullptr;

  return jni::ToJavaArray(env, g_ratingClazz, ratingCategories,
                          [](JNIEnv * env, std::string const & item) {
                            return CreateRating(env, item);
                          });
}

jintArray ToReachableByTaxiProvidersArray(JNIEnv * env, std::vector<taxi::Provider::Type> const & types)
{
  auto const count = static_cast<jsize>(types.size());

  if (count == 0)
    return nullptr;

  jint tmp[count];
  for (size_t i = 0; i < count; ++i)
    tmp[i] = static_cast<jint>(types[i]);

  jintArray result = env->NewIntArray(count);
  ASSERT(result, ());
  env->SetIntArrayRegion(result, 0, count, tmp);

  return result;
}

jobject CreateLocalAdInfo(JNIEnv * env, place_page::Info const & info)
{
  static jclass const localAdInfoClazz = jni::GetGlobalClassRef(env, "com/mapswithme/maps/ads/LocalAdInfo");
  static jmethodID const ctorId = jni::GetConstructorID(env, localAdInfoClazz,
                                                        "(ILjava/lang/String;)V");

  jni::TScopedLocalRef jLocalAdUrl(env, jni::ToJavaString(env, info.GetLocalAdsUrl()));

  return env->NewObject(localAdInfoClazz, ctorId, info.GetLocalAdsStatus(), jLocalAdUrl.get());
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
