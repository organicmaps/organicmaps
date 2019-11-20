#include "com/mapswithme/maps/promo/Promo.hpp"

#include "com/mapswithme/maps/Framework.hpp"

#include "partners_api/promo_api.hpp"

#include "geometry/mercator.hpp"

#include <utility>

using namespace std::placeholders;

namespace
{
jclass g_galleryClass = nullptr;
jclass g_itemClass = nullptr;
jclass g_placeClass = nullptr;
jclass g_authorClass = nullptr;
jclass g_categoryClass = nullptr;
jmethodID g_galleryConstructor = nullptr;
jmethodID g_itemConstructor = nullptr;
jmethodID g_placeConstructor = nullptr;
jmethodID g_authorConstructor = nullptr;
jmethodID g_categoryConstructor = nullptr;
jclass g_promoClass = nullptr;
jfieldID g_promoInstanceField = nullptr;
jmethodID g_onGalleryReceived = nullptr;
jmethodID g_onErrorReceived = nullptr;
jclass g_afterBooking = nullptr;
jmethodID g_afterBookingConstructor = nullptr;
uint64_t g_lastRequestId = 0;

void PrepareClassRefs(JNIEnv * env)
{
  if (g_galleryClass != nullptr)
    return;

  g_galleryClass = jni::GetGlobalClassRef(env, "com/mapswithme/maps/promo/PromoCityGallery");
  g_galleryConstructor =
      jni::GetConstructorID(env, g_galleryClass,
                            "([Lcom/mapswithme/maps/promo/PromoCityGallery$Item;"
                            "Ljava/lang/String;Ljava/lang/String;)V");
  g_itemClass = jni::GetGlobalClassRef(env, "com/mapswithme/maps/promo/PromoCityGallery$Item");
  g_itemConstructor =
      jni::GetConstructorID(env, g_itemClass,
                            "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;"
                            "Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;"
                            "Lcom/mapswithme/maps/promo/PromoCityGallery$Place;"
                            "Lcom/mapswithme/maps/promo/PromoCityGallery$Author;"
                            "Lcom/mapswithme/maps/promo/PromoCityGallery$LuxCategory;)V");
  g_placeClass =
    jni::GetGlobalClassRef(env, "com/mapswithme/maps/promo/PromoCityGallery$Place");
  g_placeConstructor =
    jni::GetConstructorID(env, g_placeClass, "(Ljava/lang/String;Ljava/lang/String;)V");
  g_authorClass =
      jni::GetGlobalClassRef(env, "com/mapswithme/maps/promo/PromoCityGallery$Author");
  g_authorConstructor =
      jni::GetConstructorID(env, g_authorClass, "(Ljava/lang/String;Ljava/lang/String;)V");
  g_categoryClass =
      jni::GetGlobalClassRef(env, "com/mapswithme/maps/promo/PromoCityGallery$LuxCategory");
  g_categoryConstructor =
      jni::GetConstructorID(env, g_categoryClass, "(Ljava/lang/String;Ljava/lang/String;)V");

  g_promoClass = jni::GetGlobalClassRef(env, "com/mapswithme/maps/promo/Promo");
  g_promoInstanceField =
      jni::GetStaticFieldID(env, g_promoClass, "INSTANCE", "Lcom/mapswithme/maps/promo/Promo;");
  jobject promoInstance = env->GetStaticObjectField(g_promoClass, g_promoInstanceField);
  g_onGalleryReceived = jni::GetMethodID(env, promoInstance, "onCityGalleryReceived",
                                         "(Lcom/mapswithme/maps/promo/PromoCityGallery;)V");
  g_onErrorReceived = jni::GetMethodID(env, promoInstance, "onErrorReceived", "()V");
  g_afterBooking = jni::GetGlobalClassRef(env, "com/mapswithme/maps/promo/PromoAfterBooking");
  g_afterBookingConstructor =
      jni::GetConstructorID(env, g_afterBooking,
                            "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
  jni::HandleJavaException(env);
}

void OnSuccess(uint64_t requestId, promo::CityGallery const & gallery)
{
  if (g_lastRequestId != requestId)
    return;

  JNIEnv * env = jni::GetEnv();
  jni::TScopedLocalRef cityGallery(env, promo::MakeCityGallery(env, gallery));
  jobject promoInstance = env->GetStaticObjectField(g_promoClass, g_promoInstanceField);
  env->CallVoidMethod(promoInstance, g_onGalleryReceived, cityGallery.get());

  jni::HandleJavaException(env);
}

void OnError(uint64_t requestId)
{
  if (g_lastRequestId != requestId)
    return;

  JNIEnv * env = jni::GetEnv();
  jobject promoInstance = env->GetStaticObjectField(g_promoClass, g_promoInstanceField);
  env->CallVoidMethod(promoInstance, g_onErrorReceived);

  jni::HandleJavaException(env);
}
}  // namespace

namespace promo
{
jobject MakeCityGallery(JNIEnv * env, promo::CityGallery const & gallery)
{
  PrepareClassRefs(env);

  auto const itemBuilder = [](JNIEnv * env, promo::CityGallery::Item const & item)
  {
    jni::TScopedLocalRef name(env, jni::ToJavaString(env, item.m_name));
    jni::TScopedLocalRef url(env, jni::ToJavaString(env, item.m_url));
    jni::TScopedLocalRef imageUrl(env, jni::ToJavaString(env, item.m_imageUrl));
    jni::TScopedLocalRef access(env, jni::ToJavaString(env, item.m_access));
    jni::TScopedLocalRef tier(env, jni::ToJavaString(env, item.m_tier));
    jni::TScopedLocalRef tourCategory(env, jni::ToJavaString(env, item.m_tourCategory));
    jni::TScopedLocalRef placeName(env, jni::ToJavaString(env, item.m_place.m_name));
    jni::TScopedLocalRef placeDescription(env, jni::ToJavaString(env, item.m_place.m_description));
    jni::TScopedLocalRef authorId(env, jni::ToJavaString(env, item.m_author.m_id));
    jni::TScopedLocalRef authorName(env, jni::ToJavaString(env, item.m_author.m_name));
    jni::TScopedLocalRef luxCategoryName(env, jni::ToJavaString(env, item.m_luxCategory.m_name));
    jni::TScopedLocalRef luxCategoryColor(env, jni::ToJavaString(env, item.m_luxCategory.m_color));

    jni::TScopedLocalRef place(
      env, env->NewObject(g_placeClass, g_placeConstructor, placeName.get(), placeDescription.get()));
    jni::TScopedLocalRef author(
        env, env->NewObject(g_authorClass, g_authorConstructor, authorId.get(), authorName.get()));
    jni::TScopedLocalRef luxCategory(
        env, env->NewObject(g_categoryClass, g_categoryConstructor, luxCategoryName.get(),
                            luxCategoryColor.get()));

      return env->NewObject(g_itemClass, g_itemConstructor, name.get(), url.get(), imageUrl.get(),
                            access.get(), tier.get(), tourCategory.get(), place.get(), author.get(),
                            luxCategory.get());
  };

  jni::TScopedLocalObjectArrayRef items(env, jni::ToJavaArray(env, g_itemClass, gallery.m_items,
                                        itemBuilder));
  jni::TScopedLocalRef moreUrl(env, jni::ToJavaString(env, gallery.m_moreUrl));
  jni::TScopedLocalRef category(env, jni::ToJavaString(env, gallery.m_category));

  return env->NewObject(g_galleryClass, g_galleryConstructor, items.get(), moreUrl.get(),
                        category.get());
}
}

extern "C" {
JNIEXPORT void JNICALL
Java_com_mapswithme_maps_promo_Promo_nativeRequestCityGallery(JNIEnv * env, jclass,
                                                              jobject policy, jdouble lat,
                                                              jdouble lon, jint utm)
{
  PrepareClassRefs(env);
  auto const point = mercator::FromLatLon(static_cast<double>(lat), static_cast<double>(lon));
  ++g_lastRequestId;
  g_framework->GetPromoCityGallery(env, policy, point, static_cast<UTM>(utm),
                                   std::bind(OnSuccess, g_lastRequestId, _1),
                                   std::bind(OnError, g_lastRequestId));
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_promo_Promo_nativeRequestPoiGallery(JNIEnv * env, jclass,
                                                             jobject policy, jdouble lat,
                                                             jdouble lon, jobjectArray tags,
                                                             jint utm)
{
  PrepareClassRefs(env);
  auto const point = mercator::FromLatLon(static_cast<double>(lat), static_cast<double>(lon));
  jsize const size = env->GetArrayLength(tags);
  promo::Tags nativeTags;
  for (jsize i = 0; i < size; ++i)
  {
    auto tag = jni::ToNativeString(env, static_cast<jstring>(env->GetObjectArrayElement(tags, i)));
    nativeTags.emplace_back(std::move(tag));
  }
  bool useCoordinates =
      GetPlatform().ConnectionStatus() == Platform::EConnectionType::CONNECTION_WIFI;
  ++g_lastRequestId;
  g_framework->GetPromoPoiGallery(env, policy, point, nativeTags, useCoordinates,
                                  static_cast<UTM>(utm), std::bind(OnSuccess, g_lastRequestId, _1),
                                  std::bind(OnError, g_lastRequestId));
}

JNIEXPORT jobject JNICALL
Java_com_mapswithme_maps_promo_Promo_nativeGetPromoAfterBooking(JNIEnv * env, jclass,
                                                                jobject policy)
{
  PrepareClassRefs(env);

  auto const result = g_framework->GetPromoAfterBooking(env, policy);

  if (result.IsEmpty())
    return nullptr;

  auto const id = jni::ToJavaString(env, result.m_id);
  auto const promoUrl = jni::ToJavaString(env, result.m_promoUrl);
  auto const pictureUrl = jni::ToJavaString(env, result.m_pictureUrl);


  return env->NewObject(g_afterBooking, g_afterBookingConstructor, id, promoUrl, pictureUrl);
}

JNIEXPORT jstring JNICALL
Java_com_mapswithme_maps_promo_Promo_nativeGetCityUrl(JNIEnv * env, jclass, jobject policy,
                                                      jdouble lat, jdouble lon)
{
  PrepareClassRefs(env);

  auto const cityUrl = g_framework->GetPromoCityUrl(env, policy, lat, lon);

  if (cityUrl.empty())
    return nullptr;

  return jni::ToJavaString(env, cityUrl);
}
}  // extern "C"
