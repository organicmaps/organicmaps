#include "com/mapswithme/maps/promo/CityGallery.hpp"

#include "com/mapswithme/maps/Framework.hpp"

#include "partners_api/promo_api.hpp"

#include <utility>

using namespace std::placeholders;

namespace
{
jclass m_galleryClass = nullptr;
jclass m_itemClass = nullptr;
jclass m_authorClass = nullptr;
jclass m_categoryClass = nullptr;
jmethodID m_galleryConstructor = nullptr;
jmethodID m_itemConstructor = nullptr;
jmethodID m_authorConstructor = nullptr;
jmethodID m_categoryConstructor = nullptr;
jclass g_promoClass = nullptr;
jfieldID g_promoInstanceField = nullptr;
jobject g_promoInstance = nullptr;
jmethodID g_onGalleryReceived = nullptr;
jmethodID g_onErrorReceived = nullptr;
uint64_t g_lastRequestId = 0;

void PrepareClassRefs(JNIEnv * env)
{
  if (m_galleryClass != nullptr)
    return;

  m_galleryClass = jni::GetGlobalClassRef(env, "com/mapswithme/maps/discovery/PromoCityGallery");
  m_galleryConstructor =
      jni::GetConstructorID(env, m_galleryClass,
                            "([Lcom/mapswithme/maps/discovery/PromoCityGallery$Item;"
                            "Ljava/lang/String;)V");
  m_itemClass = jni::GetGlobalClassRef(env, "com/mapswithme/maps/discovery/PromoCityGallery$Item");
  m_itemConstructor =
      jni::GetConstructorID(env, m_itemClass,
                            "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;"
                            "Ljava/lang/String;Ljava/lang/String;"
                            "Lcom/mapswithme/maps/discovery/PromoCityGallery$Author;"
                            "Lcom/mapswithme/maps/discovery/PromoCityGallery$LuxCategory;)V");
  m_authorClass =
      jni::GetGlobalClassRef(env, "com/mapswithme/maps/discovery/PromoCityGallery$Author");
  m_authorConstructor =
      jni::GetConstructorID(env, m_authorClass, "(Ljava/lang/String;Ljava/lang/String;)V");
  m_categoryClass =
      jni::GetGlobalClassRef(env, "com/mapswithme/maps/discovery/PromoCityGallery$LuxCategory");
  m_categoryConstructor =
      jni::GetConstructorID(env, m_authorClass, "(Ljava/lang/String;Ljava/lang/String;)V");
  g_promoClass = jni::GetGlobalClassRef(env, "com/mapswithme/maps/promo/Promo");

  g_promoInstanceField =
      jni::GetStaticFieldID(env, g_promoClass, "INSTANCE", "Lcom/mapswithme/maps/promo/Promo;");
  g_promoInstance = env->GetStaticObjectField(g_promoClass, g_promoInstanceField);
  g_onGalleryReceived = jni::GetMethodID(env, g_promoInstance, "onCityGalleryReceived",
                                         "(Lcom/mapswithme/maps/promo/PromoCityGallery;)V");
  g_onErrorReceived = jni::GetMethodID(env, g_promoInstance, "onErrorReceived", "()V");
}

void OnSuccess(uint64_t requestId, promo::CityGallery const & gallery)
{
  if (g_lastRequestId != requestId)
    return;

  JNIEnv * env = jni::GetEnv();
  jni::TScopedLocalRef cityGallery(env, promo::MakeCityGallery(env, gallery));
  env->CallVoidMethod(g_promoInstance, g_onGalleryReceived, cityGallery.get());

  jni::HandleJavaException(env);
}

void OnError(uint64_t requestId)
{
  if (g_lastRequestId != requestId)
    return;

  JNIEnv * env = jni::GetEnv();
  env->CallVoidMethod(g_promoInstance, g_onErrorReceived);

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
    jni::TScopedLocalRef authorId(env, jni::ToJavaString(env, item.m_author.m_id));
    jni::TScopedLocalRef authorName(env, jni::ToJavaString(env, item.m_author.m_name));
    jni::TScopedLocalRef luxCategoryName(env, jni::ToJavaString(env, item.m_luxCategory.m_name));
    jni::TScopedLocalRef luxCategoryColor(env, jni::ToJavaString(env, item.m_luxCategory.m_color));

    jni::TScopedLocalRef author(
        env, env->NewObject(m_authorClass, m_authorConstructor, authorId.get(), authorName.get()));
    jni::TScopedLocalRef luxCategory(
        env, env->NewObject(m_categoryClass, m_categoryConstructor, luxCategoryName.get(),
                            luxCategoryColor.get()));

    return env->NewObject(m_itemClass, m_itemConstructor, name.get(), url.get(), imageUrl.get(),
                          access.get(), tier.get(), author.get(), luxCategory.get());
  };

  jni::TScopedLocalRef items(env, jni::ToJavaArray(env, m_itemClass, gallery.m_items, itemBuilder));
  jni::TScopedLocalRef moreUrl(env, jni::ToJavaString(env, gallery.m_moreUrl));

  return env->NewObject(m_galleryClass, m_galleryConstructor, items.get(), moreUrl.get());
}
}

extern "C" {
JNIEXPORT void JNICALL
Java_com_mapswithme_maps_promo_Promo_nativeRequestCityGallery(JNIEnv * env, jclass,
                                                              jobject policy, jstring id)
{
  PrepareClassRefs(env);
  ++g_lastRequestId;
  g_framework->GetPromoCityGallery(env, policy, id, std::bind(OnSuccess, g_lastRequestId, _1),
                                   std::bind(OnError, g_lastRequestId));
}
}  // extern "C"
