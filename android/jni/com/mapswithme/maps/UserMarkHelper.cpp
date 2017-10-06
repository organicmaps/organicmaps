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

  return env->NewObject(g_bannerClazz, bannerCtorId, jni::ToJavaString(env, id), type);
}

jobject CreateMapObject(JNIEnv * env, string const & mwmName, int64_t mwmVersion,
                        uint32_t featureIndex, int mapObjectType, string const & title,
                        string const & secondaryTitle, string const & subtitle, double lat,
                        double lon, string const & address, Metadata const & metadata,
                        string const & apiId, jobjectArray jbanners, jintArray jTaxiTypes,
                        string const & bookingSearchUrl, jobject const & localAdInfo,
                        jobject const & routingPointInfo, bool isExtendedView, bool shouldShowUGC,
                        bool canBeRated)
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
      "Lcom/mapswithme/maps/routing/RoutePointInfo;ZZZ)V");
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
                     static_cast<jboolean>(canBeRated));

  InjectMetadata(env, g_mapObjectClazz, mapObject, metadata);
  return mapObject;
}

jobject CreateMapObject(JNIEnv * env, place_page::Info const & info)
{
  jni::TScopedLocalObjectArrayRef jbanners(env, nullptr);
  if (info.HasBanner())
    jbanners.reset(ToBannersArray(env, info.GetBanners()));

  jni::TScopedLocalIntArrayRef jTaxiTypes(env, ToReachableByTaxiProvidersArray(env, info.ReachableByTaxiProviders()));

  jni::TScopedLocalRef localAdInfo(env, CreateLocalAdInfo(env, info));

  jni::TScopedLocalRef routingPointInfo(env, nullptr);
  if (info.IsRoutePoint())
    routingPointInfo.reset(CreateRoutePointInfo(env, info));

  if (info.IsBookmark())
  {
    // public Bookmark(@NonNull FeatureId featureId,
    //                 @IntRange(from = 0) int categoryId, @IntRange(from = 0) int bookmarkId,
    //                 String title, @Nullable String secondaryTitle, @Nullable String objectTitle,
    //                 @Nullable Banner[] banners, boolean reachableByTaxi,
    //                 @Nullable String bookingSearchUrl, @Nullable LocalAdInfo localAdInfo,
    //                 @Nullable RoutePointInfo routePointInfo)
    static jmethodID const ctorId =
        jni::GetConstructorID(env, g_bookmarkClazz,
                              "(Lcom/mapswithme/maps/bookmarks/data/FeatureId;IILjava/lang/String;"
                              "Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;"
                              "[Lcom/mapswithme/maps/ads/Banner;[ILjava/lang/String;"
                              "Lcom/mapswithme/maps/ads/LocalAdInfo;"
                              "Lcom/mapswithme/maps/routing/RoutePointInfo;ZZZ)V");
    // public FeatureId(@NonNull String mwmName, long mwmVersion, int featureIndex)
    static jmethodID const featureCtorId =
        jni::GetConstructorID(env, g_featureIdClazz, "(Ljava/lang/String;JI)V");

    auto const & bac = info.GetBookmarkAndCategory();
    BookmarkCategory * cat = g_framework->NativeFramework()->GetBmCategory(bac.m_categoryIndex);
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
        g_bookmarkClazz, ctorId, jFeatureId.get(), static_cast<jint>(bac.m_categoryIndex),
        static_cast<jint>(bac.m_bookmarkIndex), jTitle.get(), jSecondaryTitle.get(), jSubtitle.get(),
        jAddress.get(), jbanners.get(), jTaxiTypes.get(), jBookingSearchUrl.get(),
        localAdInfo.get(), routingPointInfo.get(), info.IsPreviewExtended(), info.ShouldShowUGC(),
        info.CanBeRated());

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
                           info.IsPreviewExtended(), info.ShouldShowUGC(), info.CanBeRated());
  }

  if (info.HasApiUrl())
  {
    return CreateMapObject(
        env, info.GetID().GetMwmName(), info.GetID().GetMwmVersion(), info.GetID().m_index,
        kApiPoint, info.GetTitle(), info.GetSecondaryTitle(), info.GetSubtitle(), ll.lat, ll.lon,
        info.GetAddress(), info.GetMetadata(), info.GetApiUrl(), jbanners.get(), jTaxiTypes.get(),
        info.GetBookingSearchUrl(), localAdInfo.get(), routingPointInfo.get(), info.IsPreviewExtended(),
        info.ShouldShowUGC(), info.CanBeRated());
  }

  return CreateMapObject(
      env, info.GetID().GetMwmName(), info.GetID().GetMwmVersion(), info.GetID().m_index, kPoi,
      info.GetTitle(), info.GetSecondaryTitle(), info.GetSubtitle(), ll.lat, ll.lon,
      info.GetAddress(), info.IsFeature() ? info.GetMetadata() : Metadata(), "", jbanners.get(),
      jTaxiTypes.get(), info.GetBookingSearchUrl(), localAdInfo.get(), routingPointInfo.get(),
      info.IsPreviewExtended(), info.ShouldShowUGC(), info.CanBeRated());
}

jobjectArray ToBannersArray(JNIEnv * env, vector<ads::Banner> const & banners)
{
  return jni::ToJavaArray(env, g_bannerClazz, banners,
                          [](JNIEnv * env, ads::Banner const & item) {
                            return CreateBanner(env, item.m_bannerId, static_cast<jint>(item.m_type));
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
}  // namespace usermark_helper
