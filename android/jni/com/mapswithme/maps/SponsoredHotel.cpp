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

void PrepareClassRefs(JNIEnv * env, jclass hotelClass)
{
  if (g_hotelClass)
    return;

  g_hotelClass = static_cast<jclass>(env->NewGlobalRef(hotelClass));

  // SponsoredHotel(String rating, String price, String urlBook, String urlDescription)
  g_hotelClassCtor = jni::GetConstructorID(env, g_hotelClass, "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
  // static void onPriceReceived(final String id, final String price, final String currency)
  g_priceCallback = jni::GetStaticMethodID(env, g_hotelClass, "onPriceReceived", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
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

} // extern "C"
