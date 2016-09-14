#include "Framework.hpp"

#include "../core/jni_helper.hpp"
#include "../platform/Platform.hpp"
#include "map/place_page_info.hpp"

#include "std/bind.hpp"

namespace
{

jclass g_hotelClass;
jmethodID g_hotelClassCtor;
jmethodID g_priceCallback;
jmethodID g_descriptionCallback;
jmethodID g_facilitiesCallback;
jmethodID g_imagesCallback;

void PrepareClassRefs(JNIEnv * env, jclass hotelClass)
{
  if (g_hotelClass)
    return;

  g_hotelClass = static_cast<jclass>(env->NewGlobalRef(hotelClass));

  // SponsoredHotel(String rating, String price, String urlBook, String urlDescription)
  g_hotelClassCtor = jni::GetConstructorID(env, g_hotelClass, "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
  // static void onPriceReceived(final String id, final String price, final String currency)
  g_priceCallback = jni::GetStaticMethodID(env, g_hotelClass, "onPriceReceived", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
  // static void onDescriptionReceived(final String id, final String description)
  g_descriptionCallback = jni::GetStaticMethodID(env, g_hotelClass, "onDescriptionReceived", "(Ljava/lang/String;Ljava/lang/String;)V");
  // static void onFacilitiesReceived(final String id, int[] ids, String[] names)
  g_facilitiesCallback = jni::GetStaticMethodID(env, g_hotelClass, "onFacilitiesReceived", "(Ljava/lang/String;[I[Ljava/lang/String;)V");
  // static void onImagesReceived(final String id, String[] urls)
  g_imagesCallback = jni::GetStaticMethodID(env, g_hotelClass, "onImagesReceived", "(Ljava/lang/String;[Ljava/lang/String;)V");
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

// static void nativeRequestDescription(String id, String locale);
JNIEXPORT void JNICALL
Java_com_mapswithme_maps_widget_placepage_SponsoredHotel_nativeRequestDescription(JNIEnv * env, jclass clazz, jstring id, jstring locale)
{
  PrepareClassRefs(env, clazz);

  string const hotelId = jni::ToNativeString(env, id);
  string const localeCode = jni::ToNativeString(env, locale);

  //TODO make request
  //JNIEnv * env = jni::GetEnv();
  env->CallStaticVoidMethod(g_hotelClass, g_descriptionCallback, jni::ToJavaString(env, hotelId),
                                                                 jni::ToJavaString(env, "One of our top picks in New York City. This boutique hotel in the Manhattan neighborhood of Nolita features a private rooftop and rooms with free WiFi. The Bowery subway station is 1 block from this New York hotel. One of our top picks in New York City. This boutique hotel in the Manhattan neighborhood of Nolita features a private rooftop and rooms with free WiFi. The Bowery subway station is 1 block from this New York hotel."));
}

// static void nativeRequestFacilities(String id, String locale);
JNIEXPORT void JNICALL
Java_com_mapswithme_maps_widget_placepage_SponsoredHotel_nativeRequestFacilities(JNIEnv * env, jclass clazz, jstring id, jstring locale)
{
  PrepareClassRefs(env, clazz);

  string const hotelId = jni::ToNativeString(env, id);
  string const localeCode = jni::ToNativeString(env, locale);

  //TODO make request
  jintArray result;
  result = env->NewIntArray(7);
  jint fill[7]{0, 1, 2, 3, 4, 5, 6};
  env->SetIntArrayRegion(result, 0, 7, fill);
  env->CallStaticVoidMethod(g_hotelClass, g_facilitiesCallback, jni::ToJavaString(env, hotelId),
                                                                result,
                                                                jni::ToJavaStringArray(env, {"Bar", "Terrace", "Fitness Center", "Pets are allowed on request", "Restaurant", "Private parking", "Ghost Busters"}));
}

// static void nativeRequestImages(String id, String locale);
JNIEXPORT void JNICALL
Java_com_mapswithme_maps_widget_placepage_SponsoredHotel_nativeRequestImages(JNIEnv * env, jclass clazz, jstring id, jstring locale)
{
  PrepareClassRefs(env, clazz);

  string const hotelId = jni::ToNativeString(env, id);
  string const localeCode = jni::ToNativeString(env, locale);

  //TODO make request
  env->CallStaticVoidMethod(g_hotelClass, g_imagesCallback, jni::ToJavaString(env, hotelId),
                                                            jni::ToJavaStringArray(env, {"http://www.libertyhotelslara.com/dosyalar/resimler/liberty-lara-hotel1.jpg",
                                                            "https://www.omnihotels.com/-/media/images/hotels/ausctr/pool/ausctr-omni-austin-hotel-downtown-evening-pool.jpg?h=660&la=en&w=1170",
                                                            "http://www.thefloridahotelorlando.com/var/floridahotelorlando/storage/images/media/images/photo-gallery/hotel-images/florida-hotel-orlando-night/27177-1-eng-US/Florida-Hotel-Orlando-Night.jpg",
                                                            "http://www.college-hotel.com/client/cache/contenu/_500____college-hotelp1diapo1_718.jpg",
                                                            "http://top10hotelbookingsites.webs.com/besthotelsites-1.jpg",
                                                            "http://www.litorehotel.com/web/en/images/placeholders/1920x1200-0.jpg"}));
}

} // extern "C"
