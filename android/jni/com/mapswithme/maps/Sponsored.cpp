#include "Framework.hpp"

#include "../core/jni_helper.hpp"
#include "../platform/Platform.hpp"
#include "map/place_page_info.hpp"
#include "partners_api/booking_api.hpp"

#include "std/bind.hpp"
#include "std/chrono.hpp"

namespace
{
jclass g_sponsoredClass;
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
jmethodID g_sponsoredClassConstructor;
jmethodID g_priceCallback;
jmethodID g_infoCallback;

void PrepareClassRefs(JNIEnv * env, jclass sponsoredClass)
{
  if (g_sponsoredClass)
    return;

  g_sponsoredClass = static_cast<jclass>(env->NewGlobalRef(sponsoredClass));
  g_hotelInfoClass =
      jni::GetGlobalClassRef(env, "com/mapswithme/maps/widget/placepage/Sponsored$HotelInfo");
  g_facilityTypeClass = jni::GetGlobalClassRef(
      env, "com/mapswithme/maps/widget/placepage/Sponsored$FacilityType");
  g_nearbyObjectClass = jni::GetGlobalClassRef(
      env, "com/mapswithme/maps/widget/placepage/Sponsored$NearbyObject");
  g_reviewClass = jni::GetGlobalClassRef(env, "com/mapswithme/maps/review/Review");
  g_imageClass = jni::GetGlobalClassRef(env, "com/mapswithme/maps/gallery/Image");

  g_facilityConstructor =
      jni::GetConstructorID(env, g_facilityTypeClass, "(Ljava/lang/String;Ljava/lang/String;)V");
  g_nearbyConstructor = jni::GetConstructorID(
      env, g_nearbyObjectClass, "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;DD)V");
  g_imageConstructor =
      jni::GetConstructorID(env, g_imageClass, "(Ljava/lang/String;Ljava/lang/String;)V");
  g_reviewConstructor = jni::GetConstructorID(env, g_reviewClass,
                                              "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/"
                                              "String;Ljava/lang/String;Ljava/lang/String;FJ)V");
  g_hotelInfoConstructor = jni::GetConstructorID(
      env, g_hotelInfoClass,
      "(Ljava/lang/String;[Lcom/mapswithme/maps/gallery/Image;[Lcom/mapswithme/maps/widget/"
      "placepage/Sponsored$FacilityType;[Lcom/mapswithme/maps/review/Review;[Lcom/mapswithme/"
      "maps/widget/placepage/Sponsored$NearbyObject;)V");

  // Sponsored(String rating, String price, String urlBook, String urlDescription)
  g_sponsoredClassConstructor = jni::GetConstructorID(
      env, g_sponsoredClass,
      "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;I)V");
  // static void onPriceReceived(final String id, final String price, final String currency)
  g_priceCallback =
      jni::GetStaticMethodID(env, g_sponsoredClass, "onPriceReceived",
                             "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
  // static void onDescriptionReceived(final String id, final String description)
  g_infoCallback = jni::GetStaticMethodID(
      env, g_sponsoredClass, "onHotelInfoReceived",
      "(Ljava/lang/String;Lcom/mapswithme/maps/widget/placepage/Sponsored$HotelInfo;)V");
}

}  // namespace

extern "C" {

// static Sponsored nativeGetCurrent();
JNIEXPORT jobject JNICALL Java_com_mapswithme_maps_widget_placepage_Sponsored_nativeGetCurrent(
    JNIEnv * env, jclass clazz)
{
  PrepareClassRefs(env, clazz);

  place_page::Info const & ppInfo = g_framework->GetPlacePageInfo();
  if (!ppInfo.IsSponsored())
    return nullptr;

  return env->NewObject(g_sponsoredClass, g_sponsoredClassConstructor,
                        jni::ToJavaString(env, ppInfo.GetRatingFormatted()),
                        jni::ToJavaString(env, ppInfo.GetApproximatePricing()),
                        jni::ToJavaString(env, ppInfo.GetSponsoredUrl()),
                        jni::ToJavaString(env, ppInfo.GetSponsoredDescriptionUrl()),
                        (jint)ppInfo.m_sponsoredType);
}

// static void nativeRequestPrice(String id, String currencyCode);
JNIEXPORT void JNICALL Java_com_mapswithme_maps_widget_placepage_Sponsored_nativeRequestPrice(
    JNIEnv * env, jclass clazz, jstring id, jstring currencyCode)
{
  PrepareClassRefs(env, clazz);

  string const hotelId = jni::ToNativeString(env, id);
  string const code = jni::ToNativeString(env, currencyCode);

  g_framework->RequestBookingMinPrice(hotelId, code, [hotelId](string const & price,
                                                               string const & currency) {
    GetPlatform().RunOnGuiThread([=]() {
      JNIEnv * env = jni::GetEnv();
      env->CallStaticVoidMethod(g_sponsoredClass, g_priceCallback, jni::ToJavaString(env, hotelId),
                                jni::ToJavaString(env, price), jni::ToJavaString(env, currency));
    });
  });
}

// static void nativeRequestInfo(String id, String locale);
JNIEXPORT void JNICALL Java_com_mapswithme_maps_widget_placepage_Sponsored_nativeRequestHotelInfo(
    JNIEnv * env, jclass clazz, jstring id, jstring locale)
{
  PrepareClassRefs(env, clazz);

  string const hotelId = jni::ToNativeString(env, id);
  string const code = jni::ToNativeString(env, locale);

  g_framework->RequestBookingInfo(hotelId, code, [hotelId](
                                                     BookingApi::HotelInfo const & hotelInfo) {
    GetPlatform().RunOnGuiThread([=]() {
      JNIEnv * env = jni::GetEnv();

      auto description = jni::ToJavaString(env, hotelInfo.m_description);
      auto photos =
          jni::ToJavaArray(env, g_imageClass, hotelInfo.m_photos,
                           [](JNIEnv * env, BookingApi::HotelPhotoUrls const & item) {
                             return env->NewObject(g_imageClass, g_imageConstructor,
                                                   jni::ToJavaString(env, item.m_original),
                                                   jni::ToJavaString(env, item.m_small));
                           });
      auto facilities =
          jni::ToJavaArray(env, g_facilityTypeClass, hotelInfo.m_facilities,
                           [](JNIEnv * env, BookingApi::Facility const & item) {
                             return env->NewObject(g_facilityTypeClass, g_facilityConstructor,
                                                   jni::ToJavaString(env, item.m_id),
                                                   jni::ToJavaString(env, item.m_localizedName));
                           });
      auto reviews = jni::ToJavaArray(
          env, g_reviewClass, hotelInfo.m_reviews,
          [](JNIEnv * env, BookingApi::HotelReview const & item) {
            return env->NewObject(
                g_reviewClass, g_reviewConstructor, jni::ToJavaString(env, item.m_reviewNeutral),
                jni::ToJavaString(env, item.m_reviewPositive),
                jni::ToJavaString(env, item.m_reviewNegative),
                jni::ToJavaString(env, item.m_author), jni::ToJavaString(env, item.m_authorPictUrl),
                item.m_rating,
                time_point_cast<milliseconds>(item.m_date).time_since_epoch().count());
          });
      auto nearby = env->NewObjectArray(0, g_nearbyObjectClass, 0);

      env->CallStaticVoidMethod(g_sponsoredClass, g_infoCallback, jni::ToJavaString(env, hotelId),
                                env->NewObject(g_hotelInfoClass, g_hotelInfoConstructor,
                                               description, photos, facilities, reviews, nearby));
    });
  });
}

}  // extern "C"
