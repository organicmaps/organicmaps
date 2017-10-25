#include "Framework.hpp"

#include "android/jni/com/mapswithme/core/jni_helper.hpp"
#include "android/jni/com/mapswithme/platform/Platform.hpp"
#include "map/place_page_info.hpp"
#include "partners_api/booking_api.hpp"

#include <chrono>
#include <functional>

namespace
{
using namespace booking;

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
std::string g_lastRequestedHotelId;

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
                                              "(JFLjava/lang/String;Ljava/lang/"
                                              "String;Ljava/lang/String;)V");
  g_hotelInfoConstructor = jni::GetConstructorID(
      env, g_hotelInfoClass,
      "(Ljava/lang/String;[Lcom/mapswithme/maps/gallery/Image;[Lcom/mapswithme/maps/widget/"
      "placepage/Sponsored$FacilityType;[Lcom/mapswithme/maps/review/Review;[Lcom/mapswithme/"
      "maps/widget/placepage/Sponsored$NearbyObject;J)V");

  // Sponsored(String rating, String price, String urlBook, String urlDescription)
  g_sponsoredClassConstructor = jni::GetConstructorID(
      env, g_sponsoredClass,
      "(Ljava/lang/String;ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;I)V");
  // static void onPriceReceived(final String id, final String price, final String currency)
  g_priceCallback =
      jni::GetStaticMethodID(env, g_sponsoredClass, "onPriceReceived",
                             "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
  // static void onDescriptionReceived(final String id, final String description)
  g_infoCallback = jni::GetStaticMethodID(
      env, g_sponsoredClass, "onHotelInfoReceived",
      "(Ljava/lang/String;Lcom/mapswithme/maps/widget/placepage/Sponsored$HotelInfo;)V");
}

jobjectArray ToPhotosArray(JNIEnv * env, vector<HotelPhotoUrls> const & photos)
{
  return jni::ToJavaArray(env, g_imageClass, photos,
                          [](JNIEnv * env, HotelPhotoUrls const & item) {
                            return env->NewObject(g_imageClass, g_imageConstructor,
                                                  jni::ToJavaString(env, item.m_original),
                                                  jni::ToJavaString(env, item.m_small));
                          });
}

jobjectArray ToFacilitiesArray(JNIEnv * env, vector<HotelFacility> const & facilities)
{
  return jni::ToJavaArray(env, g_facilityTypeClass, facilities,
                          [](JNIEnv * env, HotelFacility const & item) {
                            return env->NewObject(g_facilityTypeClass, g_facilityConstructor,
                                                  jni::ToJavaString(env, item.m_type),
                                                  jni::ToJavaString(env, item.m_name));
                          });
}

jobjectArray ToReviewsArray(JNIEnv * env, vector<HotelReview> const & reviews)
{
  return jni::ToJavaArray(env, g_reviewClass, reviews,
                          [](JNIEnv * env, HotelReview const & item) {
                            return env->NewObject(
                                g_reviewClass, g_reviewConstructor,
                                std::chrono::time_point_cast<std::chrono::milliseconds>(item.m_date).time_since_epoch().count(),
                                item.m_score, jni::ToJavaString(env, item.m_author),
                                jni::ToJavaString(env, item.m_pros), jni::ToJavaString(env, item.m_cons));
                          });
}
}  // namespace

extern "C"
{
// static Sponsored nativeGetCurrent();
JNIEXPORT jobject JNICALL Java_com_mapswithme_maps_widget_placepage_Sponsored_nativeGetCurrent(
    JNIEnv * env, jclass clazz)
{
  PrepareClassRefs(env, clazz);

  place_page::Info const & ppInfo = g_framework->GetPlacePageInfo();
  if (!ppInfo.IsSponsored())
    return nullptr;

  std::string rating = place_page::rating::GetRatingFormatted(ppInfo.GetRatingRawValue());
  return env->NewObject(g_sponsoredClass, g_sponsoredClassConstructor,
                        jni::ToJavaString(env, rating),
                        static_cast<int>(place_page::rating::GetImpress(ppInfo.GetRatingRawValue())),
                        jni::ToJavaString(env, ppInfo.GetApproximatePricing()),
                        jni::ToJavaString(env, ppInfo.GetSponsoredUrl()),
                        jni::ToJavaString(env, ppInfo.GetSponsoredDescriptionUrl()),
                        jni::ToJavaString(env, ppInfo.GetSponsoredReviewUrl()),
                        (jint)ppInfo.GetSponsoredType());
}

// static void nativeRequestPrice(String id, String currencyCode);
JNIEXPORT void JNICALL Java_com_mapswithme_maps_widget_placepage_Sponsored_nativeRequestPrice(
    JNIEnv * env, jclass clazz, jobject policy, jstring id, jstring currencyCode)
{
  PrepareClassRefs(env, clazz);

  std::string const hotelId = jni::ToNativeString(env, id);
  g_lastRequestedHotelId = hotelId;

  std::string const code = jni::ToNativeString(env, currencyCode);

  g_framework->RequestBookingMinPrice(
      env, policy, hotelId, code,
      [](std::string const hotelId, std::string const price, std::string const currency) {
        if (g_lastRequestedHotelId != hotelId)
          return;

        JNIEnv * env = jni::GetEnv();
        env->CallStaticVoidMethod(g_sponsoredClass, g_priceCallback, jni::ToJavaString(env, hotelId),
                                  jni::ToJavaString(env, price), jni::ToJavaString(env, currency));
      });
}

// static void nativeRequestInfo(String id, String locale);
JNIEXPORT void JNICALL Java_com_mapswithme_maps_widget_placepage_Sponsored_nativeRequestHotelInfo(
    JNIEnv * env, jclass clazz, jobject policy, jstring id, jstring locale)
{
  PrepareClassRefs(env, clazz);

  std::string const hotelId = jni::ToNativeString(env, id);
  g_lastRequestedHotelId = hotelId;

  std::string code = jni::ToNativeString(env, locale);

  if (code.size() > 2)  // 2 - number of characters in country code
    code.resize(2);

  g_framework->RequestBookingInfo(env, policy, hotelId, code, [hotelId](HotelInfo const hotelInfo) {
    if (g_lastRequestedHotelId != hotelId)
      return;
    JNIEnv * env = jni::GetEnv();

    auto description = jni::ToJavaString(env, hotelInfo.m_description);
    auto photos = ToPhotosArray(env, hotelInfo.m_photos);
    auto facilities = ToFacilitiesArray(env, hotelInfo.m_facilities);
    auto reviews = ToReviewsArray(env, hotelInfo.m_reviews);
    auto nearby = env->NewObjectArray(0, g_nearbyObjectClass, 0);
    jlong reviewsCount = static_cast<jlong>(hotelInfo.m_scoreCount);
    env->CallStaticVoidMethod(g_sponsoredClass, g_infoCallback, jni::ToJavaString(env, hotelId),
                              env->NewObject(g_hotelInfoClass, g_hotelInfoConstructor, description,
                                             photos, facilities, reviews, nearby, reviewsCount));
  });
}
}  // extern "C"
