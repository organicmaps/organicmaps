#include "Framework.hpp"

#include "../core/jni_helper.hpp"
#include "../platform/Platform.hpp"
#include "map/place_page_info.hpp"
#include "map/booking_api.hpp"

#include "std/bind.hpp"

namespace
{

jclass g_hotelClass;
jclass g_facilityTypeClass;
jclass g_nearbyObjectClass;
jclass g_imageClass;
jclass g_reviewClass;
jclass g_hotelInfoClass;
jmethodID g_facilityConstructor;
jmethodID g_nearbyConstructor;
jmethodID g_imageConstructor;
jmethodID g_reviewConstructor;
jmethodID g_hotelInfoConstructor;
jmethodID g_hotelClassCtor;
jmethodID g_priceCallback;
jmethodID g_infoCallback;

void PrepareClassRefs(JNIEnv * env, jclass hotelClass)
{
  if (g_hotelClass)
    return;

  g_hotelClass = static_cast<jclass>(env->NewGlobalRef(hotelClass));
  g_hotelInfoClass = jni::GetGlobalClassRef(env, "com/mapswithme/maps/widget/placepage/SponsoredHotel$HotelInfo");
  g_facilityTypeClass = jni::GetGlobalClassRef(env, "com/mapswithme/maps/widget/placepage/SponsoredHotel$FacilityType");
  g_nearbyObjectClass = jni::GetGlobalClassRef(env, "com/mapswithme/maps/widget/placepage/SponsoredHotel$NearbyObject");
  g_reviewClass = jni::GetGlobalClassRef(env, "com/mapswithme/maps/review/Review");
  g_imageClass = jni::GetGlobalClassRef(env, "com/mapswithme/maps/gallery/Image");

  g_facilityConstructor = jni::GetConstructorID(env, g_facilityTypeClass, "(Ljava/lang/String;Ljava/lang/String;)V");
  g_nearbyConstructor = jni::GetConstructorID(env, g_nearbyObjectClass, "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;DD)V");
  g_imageConstructor = jni::GetConstructorID(env, g_imageClass, "(Ljava/lang/String;Ljava/lang/String;)V");
  g_reviewConstructor = jni::GetConstructorID(env, g_reviewClass, "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;FJ)V");
  g_hotelInfoConstructor = jni::GetConstructorID(env, g_hotelInfoClass, "(Ljava/lang/String;[Lcom/mapswithme/maps/gallery/Image;[Lcom/mapswithme/maps/widget/placepage/SponsoredHotel$FacilityType;[Lcom/mapswithme/maps/review/Review;[Lcom/mapswithme/maps/widget/placepage/SponsoredHotel$NearbyObject;)V");

  // SponsoredHotel(String rating, String price, String urlBook, String urlDescription)
  g_hotelClassCtor = jni::GetConstructorID(env, g_hotelClass, "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
  // static void onPriceReceived(final String id, final String price, final String currency)
  g_priceCallback = jni::GetStaticMethodID(env, g_hotelClass, "onPriceReceived", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
  // static void onDescriptionReceived(final String id, final String description)
  g_infoCallback = jni::GetStaticMethodID(env, g_hotelClass, "onInfoReceived", "(Ljava/lang/String;Lcom/mapswithme/maps/widget/placepage/SponsoredHotel$HotelInfo;)V");
}

} // namespace

extern "C"
{

// static SponsoredHotel nativeGetCurrent();
JNIEXPORT jobject JNICALL
Java_com_mapswithme_maps_widget_placepage_SponsoredHotel_nativeGetCurrent(JNIEnv * env, jclass clazz)
{
  PrepareClassRefs(env, clazz);

  place_page::Info const & ppInfo = g_framework->GetPlacePageInfo();
  if (!ppInfo.m_isSponsoredHotel)
    return nullptr;

  return env->NewObject(g_hotelClass, g_hotelClassCtor, jni::ToJavaString(env, ppInfo.GetRatingFormatted()),
                                                        jni::ToJavaString(env, ppInfo.GetApproximatePricing()),
                                                        jni::ToJavaString(env, ppInfo.GetSponsoredBookingUrl()),
                                                        jni::ToJavaString(env, ppInfo.GetSponsoredDescriptionUrl()));
}

// static void nativeRequestPrice(String id, String currencyCode);
JNIEXPORT void JNICALL
Java_com_mapswithme_maps_widget_placepage_SponsoredHotel_nativeRequestPrice(JNIEnv * env, jclass clazz, jstring id, jstring currencyCode)
{
  PrepareClassRefs(env, clazz);

  string const hotelId = jni::ToNativeString(env, id);
  string const code = jni::ToNativeString(env, currencyCode);

  g_framework->RequestBookingMinPrice(hotelId, code, [hotelId](string const & price, string const & currency)
  {
    GetPlatform().RunOnGuiThread([=]()
    {
      JNIEnv * env = jni::GetEnv();
      env->CallStaticVoidMethod(g_hotelClass, g_priceCallback, jni::ToJavaString(env, hotelId),
                                                               jni::ToJavaString(env, price),
                                                               jni::ToJavaString(env, currency));
    });
  });
}

// static void nativeRequestInfo(String id, String locale);
JNIEXPORT void JNICALL
Java_com_mapswithme_maps_widget_placepage_SponsoredHotel_nativeRequestInfo(JNIEnv * env, jclass clazz, jstring id, jstring locale)
{
  PrepareClassRefs(env, clazz);

  string const hotelId = jni::ToNativeString(env, id);
  string const code = jni::ToNativeString(env, locale);

  g_framework->RequestBookingInfo(hotelId, code, [hotelId](BookingApi::HotelInfo const & hotelInfo)
  {
    GetPlatform().RunOnGuiThread([=]()
    {
      JNIEnv * env = jni::GetEnv();

      env->CallStaticVoidMethod(g_hotelClass, g_infoCallback, jni::ToJavaString(env, hotelId),
          env->NewObject(g_hotelInfoClass, g_hotelInfoConstructor,
                jni::ToJavaString(env, hotelInfo.m_description),
                jni::ToJavaArray(env, g_imageClass, hotelInfo.m_photos,
                                                 [](JNIEnv * env, BookingApi::HotelPhotoUrls const & item)
                                                 {
                                                   return env->NewObject(g_imageClass, g_imageConstructor,
                                                        jni::ToJavaString(env, item.m_original),
                                                        jni::ToJavaString(env, item.m_small));
                                                 }),
                jni::ToJavaArray(env, g_facilityTypeClass, hotelInfo.m_facilities,
                                                 [](JNIEnv * env, BookingApi::Facility const & item)
                                                 {
                                                   return env->NewObject(g_facilityTypeClass, g_facilityConstructor,
                                                        jni::ToJavaString(env, item.m_id),
                                                        jni::ToJavaString(env, item.m_localizedName));
                                                 }),
                jni::ToJavaArray(env, g_reviewClass, hotelInfo.m_reviews,
                                                 [](JNIEnv * env, BookingApi::HotelReview const & item)
                                                 {
                                                   return env->NewObject(g_reviewClass, g_reviewConstructor,
                                                        jni::ToJavaString(env, item.m_reviewNeutral),
                                                        jni::ToJavaString(env, item.m_reviewPositive),
                                                        jni::ToJavaString(env, item.m_reviewNegative),
                                                        jni::ToJavaString(env, item.m_author),
                                                        jni::ToJavaString(env, item.m_authorPictUrl),
                                                        item.m_rating,
                                                        std::chrono::time_point_cast<std::chrono::milliseconds>(item.m_date).time_since_epoch().count());
                                                 }),
                env->NewObjectArray(0, g_nearbyObjectClass, 0)));
    });
  });
}

} // extern "C"
