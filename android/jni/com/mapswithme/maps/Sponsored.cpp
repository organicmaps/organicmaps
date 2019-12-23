#include "com/mapswithme/maps/Framework.hpp"

#include "com/mapswithme/core/jni_helper.hpp"
#include "com/mapswithme/platform/Platform.hpp"

#include "map/place_page_info.hpp"

#include "partners_api/booking_api.hpp"
#include "partners_api/booking_block_params.hpp"

#include <chrono>
#include <functional>
#include <utility>

namespace
{
using namespace booking;

jclass g_sponsoredClass;
jclass g_facilityTypeClass;
jclass g_nearbyObjectClass;
jclass g_imageClass;
jclass g_reviewClass;
jclass g_hotelInfoClass;
jclass g_priceInfoClass;
jmethodID g_priceInfoConstructor;
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
  g_priceInfoClass = jni::GetGlobalClassRef(env, "com/mapswithme/maps/widget/placepage/HotelPriceInfo");

  g_facilityConstructor =
      jni::GetConstructorID(env, g_facilityTypeClass, "(Ljava/lang/String;Ljava/lang/String;)V");
  g_nearbyConstructor = jni::GetConstructorID(
      env, g_nearbyObjectClass, "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;DD)V");
  g_imageConstructor =
      jni::GetConstructorID(env, g_imageClass, "(Ljava/lang/String;Ljava/lang/String;)V");
  g_reviewConstructor = jni::GetConstructorID(env, g_reviewClass,
                                              "(JFLjava/lang/String;Ljava/lang/"
                                              "String;Ljava/lang/String;)V");
  g_priceInfoConstructor =
    jni::GetConstructorID(env, g_priceInfoClass, "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;IZ)V");

  g_hotelInfoConstructor = jni::GetConstructorID(
      env, g_hotelInfoClass,
      "(Ljava/lang/String;[Lcom/mapswithme/maps/gallery/Image;[Lcom/mapswithme/maps/widget/"
      "placepage/Sponsored$FacilityType;[Lcom/mapswithme/maps/review/Review;[Lcom/mapswithme/"
      "maps/widget/placepage/Sponsored$NearbyObject;J)V");

  //  Sponsored(String rating, int impress, String price, String url, String deepLink,
  //            String descriptionUrl, String moreUrl, String reviewUrl, int type,
  //            int partnerIndex, String partnerName)
  g_sponsoredClassConstructor = jni::GetConstructorID(
      env, g_sponsoredClass,
      "(Ljava/lang/String;ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;"
      "Ljava/lang/String;Ljava/lang/String;IILjava/lang/String;)V");
  // static void onPriceReceived(final String id, final String price, final String currency)
  g_priceCallback =
      jni::GetStaticMethodID(env, g_sponsoredClass, "onPriceReceived",
                             "(Lcom/mapswithme/maps/widget/placepage/HotelPriceInfo;)V");
  // static void onDescriptionReceived(final String id, final String description)
  g_infoCallback = jni::GetStaticMethodID(
      env, g_sponsoredClass, "onHotelInfoReceived",
      "(Ljava/lang/String;Lcom/mapswithme/maps/widget/placepage/Sponsored$HotelInfo;)V");
}

jobjectArray ToPhotosArray(JNIEnv * env, std::vector<HotelPhotoUrls> const & photos)
{
  return jni::ToJavaArray(env, g_imageClass, photos,
                          [](JNIEnv * env, HotelPhotoUrls const & item) {
                            return env->NewObject(g_imageClass, g_imageConstructor,
                                                  jni::ToJavaString(env, item.m_original),
                                                  jni::ToJavaString(env, item.m_small));
                          });
}

jobjectArray ToFacilitiesArray(JNIEnv * env, std::vector<HotelFacility> const & facilities)
{
  return jni::ToJavaArray(env, g_facilityTypeClass, facilities,
                          [](JNIEnv * env, HotelFacility const & item) {
                            return env->NewObject(g_facilityTypeClass, g_facilityConstructor,
                                                  jni::ToJavaString(env, item.m_type),
                                                  jni::ToJavaString(env, item.m_name));
                          });
}

jobjectArray ToReviewsArray(JNIEnv * env, std::vector<HotelReview> const & reviews)
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

  if (!g_framework->NativeFramework()->HasPlacePageInfo())
    return nullptr;

  place_page::Info const & ppInfo = g_framework->GetPlacePageInfo();
  if (!ppInfo.IsSponsored())
    return nullptr;

  std::string rating = place_page::rating::GetRatingFormatted(ppInfo.GetRatingRawValue());
  return env->NewObject(g_sponsoredClass, g_sponsoredClassConstructor,
                        jni::ToJavaString(env, rating),
                        static_cast<jint>(place_page::rating::GetImpress(ppInfo.GetRatingRawValue())),
                        jni::ToJavaString(env, ppInfo.GetApproximatePricing()),
                        jni::ToJavaString(env, ppInfo.GetSponsoredUrl()),
                        jni::ToJavaString(env, ppInfo.GetSponsoredDeepLink()),
                        jni::ToJavaString(env, ppInfo.GetSponsoredDescriptionUrl()),
                        jni::ToJavaString(env, ppInfo.GetSponsoredMoreUrl()),
                        jni::ToJavaString(env, ppInfo.GetSponsoredReviewUrl()),
                        static_cast<jint>(ppInfo.GetSponsoredType()),
                        static_cast<jint>(ppInfo.GetPartnerIndex()),
                        jni::ToJavaString(env, ppInfo.GetPartnerName()));
}

// static void nativeRequestPrice(String id, String currencyCode);
JNIEXPORT void JNICALL Java_com_mapswithme_maps_widget_placepage_Sponsored_nativeRequestPrice(
    JNIEnv * env, jclass clazz, jobject policy, jstring id, jstring currencyCode)
{
  PrepareClassRefs(env, clazz);

  std::string const hotelId = jni::ToNativeString(env, id);
  g_lastRequestedHotelId = hotelId;

  std::string const code = jni::ToNativeString(env, currencyCode);
  auto params = booking::BlockParams::MakeDefault();
  params.m_hotelId = hotelId;
  params.m_currency = code;
  g_framework->RequestBookingMinPrice(
      env, policy, std::move(params),
      [](std::string const & hotelId, booking::Blocks const & blocks) {
        if (g_lastRequestedHotelId != hotelId)
          return;

        JNIEnv * env = jni::GetEnv();
        auto const price = blocks.m_totalMinPrice == BlockInfo::kIncorrectPrice
                           ? ""
                           : std::to_string(blocks.m_totalMinPrice);
        auto const hotelPriceInfo = env->NewObject(g_priceInfoClass,
                                                   g_priceInfoConstructor,
                                                   jni::ToJavaString(env, hotelId),
                                                   jni::ToJavaString(env, price),
                                                   jni::ToJavaString(env, blocks.m_currency),
                                                   static_cast<jint>(blocks.m_maxDiscount),
                                                   static_cast<jboolean>(blocks.m_hasSmartDeal));

        env->CallStaticVoidMethod(g_sponsoredClass, g_priceCallback, hotelPriceInfo);
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
