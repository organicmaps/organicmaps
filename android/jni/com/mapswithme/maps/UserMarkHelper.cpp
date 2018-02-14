#include "UserMarkHelper.hpp"

#include "map/place_page_info.hpp"

#include "partners_api/ads_engine.hpp"
#include "partners_api/banner.hpp"

#include "base/string_utils.hpp"

namespace usermark_helper
{
using search::AddressInfo;
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

jobject CreateMapObject(JNIEnv * env, string const & mwmName, int64_t mwmVersion,
                        uint32_t featureIndex, int mapObjectType, string const & title,
                        string const & secondaryTitle, string const & subtitle, double lat,
                        double lon, string const & address, Metadata const & metadata,
                        string const & apiId, jobjectArray jbanners, jintArray jTaxiTypes,
                        string const & bookingSearchUrl, jobject const & localAdInfo,
                        jobject const & routingPointInfo, bool isExtendedView, bool shouldShowUGC,
                        bool canBeRated, bool canBeReviewed, jobjectArray jratings)
{
  // public MapObject(@NonNull FeatureId featureId,
  //                  @MapObjectType int mapObjectType, String title, @Nullable String
  //                  secondaryTitle,
  //                  String subtitle, String address, double lat, double lon, String apiId,
  //                  @Nullable Banner[] banners, @TaxiType int[] reachableByTaxiTypes,
  //                  @Nullable String bookingSearchUrl, @Nullable LocalAdInfo localAdInfo,
  //                  @Nullable RoutePointInfo routePointInfo)
  static jmethodID const ctorId = jni::GetConstructorID(
      env, g_mapObjectClazz,
      "(Lcom/mapswithme/maps/bookmarks/data/FeatureId;ILjava/lang/String;Ljava/lang/"
      "String;Ljava/lang/String;Ljava/lang/String;DDLjava/lang/"
      "String;[Lcom/mapswithme/maps/ads/Banner;[ILjava/lang/String;"
      "Lcom/mapswithme/maps/ads/LocalAdInfo;"
      "Lcom/mapswithme/maps/routing/RoutePointInfo;ZZZZ[Lcom/mapswithme/maps/ugc/UGC$Rating;)V");
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
  jobject mapObject =
      env->NewObject(g_mapObjectClazz, ctorId, jFeatureId.get(), mapObjectType, jTitle.get(),
                     jSecondaryTitle.get(), jSubtitle.get(), jAddress.get(), lat, lon, jApiId.get(),
                     jbanners, jTaxiTypes, jBookingSearchUrl.get(), localAdInfo, routingPointInfo,
                     static_cast<jboolean>(isExtendedView), static_cast<jboolean>(shouldShowUGC),
                     static_cast<jboolean>(canBeRated), static_cast<jboolean>(canBeReviewed),
                     jratings);

  InjectMetadata(env, g_mapObjectClazz, mapObject, metadata);
  return mapObject;
}

jobject CreateMapObject(JNIEnv * env, place_page::Info const & info)
{
  jni::TScopedLocalObjectArrayRef jbanners(env, nullptr);
  if (info.HasBanner())
    jbanners.reset(ToBannersArray(env, info.GetBanners()));

  jni::TScopedLocalObjectArrayRef jratings(env, ToRatingArray(env, info.GetRatingCategories()));

  jni::TScopedLocalIntArrayRef jTaxiTypes(env, ToReachableByTaxiProvidersArray(env, info.ReachableByTaxiProviders()));

  jni::TScopedLocalRef localAdInfo(env, CreateLocalAdInfo(env, info));

  jni::TScopedLocalRef routingPointInfo(env, nullptr);
  if (info.IsRoutePoint())
    routingPointInfo.reset(CreateRoutePointInfo(env, info));

  if (info.IsBookmark())
  {
    // public Bookmark(@NonNull FeatureId featureId,
    //                 @IntRange(from = 0) long categoryId, @IntRange(from = 0) long bookmarkId,
    //                 String title, @Nullable String secondaryTitle, @Nullable String objectTitle,
    //                 @Nullable Banner[] banners, boolean reachableByTaxi,
    //                 @Nullable String bookingSearchUrl, @Nullable LocalAdInfo localAdInfo,
    //                 @Nullable RoutePointInfo routePointInfo)
    static jmethodID const ctorId =
        jni::GetConstructorID(env, g_bookmarkClazz,
                              "(Lcom/mapswithme/maps/bookmarks/data/FeatureId;JJLjava/lang/String;"
                              "Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;"
                              "[Lcom/mapswithme/maps/ads/Banner;[ILjava/lang/String;"
                              "Lcom/mapswithme/maps/ads/LocalAdInfo;"
                              "Lcom/mapswithme/maps/routing/RoutePointInfo;ZZZZ[Lcom/mapswithme/maps/ugc/UGC$Rating;)V");
    // public FeatureId(@NonNull String mwmName, long mwmVersion, int featureIndex)
    static jmethodID const featureCtorId =
        jni::GetConstructorID(env, g_featureIdClazz, "(Ljava/lang/String;JI)V");

    auto const bookmarkId = info.GetBookmarkId();
    auto const categoryId = info.GetBookmarkCategoryId();
    BookmarkData const & data = info.GetBookmarkData();
    jni::TScopedLocalRef jMwmName(env, jni::ToJavaString(env, info.GetID().GetMwmName()));
    jni::TScopedLocalRef jFeatureId(
        env, env->NewObject(g_featureIdClazz, featureCtorId, jMwmName.get(),
                            (jlong)info.GetID().GetMwmVersion(), (jint)info.GetID().m_index));
    jni::TScopedLocalRef jName(env, jni::ToJavaString(env, data.GetName()));
    jni::TScopedLocalRef jTitle(env, jni::ToJavaString(env, info.GetTitle()));
    jni::TScopedLocalRef jSecondaryTitle(env, jni::ToJavaString(env, info.GetSecondaryTitle()));
    jni::TScopedLocalRef jSubtitle(env, jni::ToJavaString(env, info.GetSubtitle()));
    jni::TScopedLocalRef jAddress(env, jni::ToJavaString(env, info.GetAddress()));

    jni::TScopedLocalRef jBookingSearchUrl(env, jni::ToJavaString(env, info.GetBookingSearchUrl()));
    jobject mapObject = env->NewObject(
        g_bookmarkClazz, ctorId, jFeatureId.get(), static_cast<jlong>(categoryId),
        static_cast<jlong>(bookmarkId), jTitle.get(), jSecondaryTitle.get(), jSubtitle.get(),
        jAddress.get(), jbanners.get(), jTaxiTypes.get(), jBookingSearchUrl.get(),
        localAdInfo.get(), routingPointInfo.get(), info.IsPreviewExtended(), info.ShouldShowUGC(),
        info.CanBeRated(), info.CanBeReviewed(), jratings.get());

    if (info.IsFeature())
      InjectMetadata(env, g_mapObjectClazz, mapObject, info.GetMetadata());
    return mapObject;
  }

  ms::LatLon const ll = info.GetLatLon();
  // TODO(yunikkk): object can be POI + API + search result + bookmark simultaneously.
  // TODO(yunikkk): Should we pass localized strings here and in other methods as byte arrays?
  if (info.IsMyPosition())
  {
    return CreateMapObject(env, info.GetID().GetMwmName(), info.GetID().GetMwmVersion(),
                           info.GetID().m_index, kMyPosition, info.GetTitle(),
                           info.GetSecondaryTitle(), info.GetSubtitle(), ll.lat, ll.lon,
                           info.GetAddress(), {}, "", jbanners.get(), jTaxiTypes.get(),
                           info.GetBookingSearchUrl(), localAdInfo.get(), routingPointInfo.get(),
                           info.IsPreviewExtended(), info.ShouldShowUGC(), info.CanBeRated(),
                           info.CanBeReviewed(), jratings.get());
  }

  if (info.HasApiUrl())
  {
    return CreateMapObject(
        env, info.GetID().GetMwmName(), info.GetID().GetMwmVersion(), info.GetID().m_index,
        kApiPoint, info.GetTitle(), info.GetSecondaryTitle(), info.GetSubtitle(), ll.lat, ll.lon,
        info.GetAddress(), info.GetMetadata(), info.GetApiUrl(), jbanners.get(), jTaxiTypes.get(),
        info.GetBookingSearchUrl(), localAdInfo.get(), routingPointInfo.get(), info.IsPreviewExtended(),
        info.ShouldShowUGC(), info.CanBeRated(), info.CanBeReviewed(), jratings.get());
  }

  return CreateMapObject(
      env, info.GetID().GetMwmName(), info.GetID().GetMwmVersion(), info.GetID().m_index, kPoi,
      info.GetTitle(), info.GetSecondaryTitle(), info.GetSubtitle(), ll.lat, ll.lon,
      info.GetAddress(), info.IsFeature() ? info.GetMetadata() : Metadata(), "", jbanners.get(),
      jTaxiTypes.get(), info.GetBookingSearchUrl(), localAdInfo.get(), routingPointInfo.get(),
      info.IsPreviewExtended(), info.ShouldShowUGC(), info.CanBeRated(), info.CanBeReviewed(),
      jratings.get());
}

jobjectArray ToBannersArray(JNIEnv * env, vector<ads::Banner> const & banners)
{
  return jni::ToJavaArray(env, g_bannerClazz, banners,
                          [](JNIEnv * env, ads::Banner const & item) {
                            return CreateBanner(env, item.m_bannerId, static_cast<jint>(item.m_type));
                          });
}

jobjectArray ToRatingArray(JNIEnv * env, vector<std::string> const & ratingCategories)
{
  if (ratingCategories.empty())
    return nullptr;

  return jni::ToJavaArray(env, g_ratingClazz, ratingCategories,
                          [](JNIEnv * env, std::string const & item) {
                            return CreateRating(env, item);
                          });
}

jintArray ToReachableByTaxiProvidersArray(JNIEnv * env, vector<taxi::Provider::Type> const & types)
{
  if (types.size() == 0)
    return nullptr;

  jintArray result = env->NewIntArray(types.size());
  ASSERT(result, ());

  jint tmp[types.size()];
  for (int i = 0; i < types.size(); i++)
    tmp[i] = static_cast<jint>(types[i]);

  env->SetIntArrayRegion(result, 0, types.size(), tmp);

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

jobjectArray ToFeatureIdArray(JNIEnv * env, vector<FeatureID> const & ids)
{
  if (ids.empty())
    return nullptr;

  return jni::ToJavaArray(env, g_featureIdClazz, ids,
                          [](JNIEnv * env, FeatureID const & fid) {
                            return CreateFeatureId(env, fid);
                          });
}
}  // namespace usermark_helper
