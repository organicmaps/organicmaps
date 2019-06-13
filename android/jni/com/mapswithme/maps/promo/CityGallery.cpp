#include "com/mapswithme/maps/promo/CityGallery.hpp"

#include "com/mapswithme/maps/Framework.hpp"

#include "partners_api/promo_api.hpp"

#include <utility>

using namespace std::placeholders;

namespace
{
jclass g_galleryClass = nullptr;
jclass g_itemClass = nullptr;
jclass g_authorClass = nullptr;
jclass g_categoryClass = nullptr;
jmethodID g_galleryConstructor = nullptr;
jmethodID g_itemConstructor = nullptr;
jmethodID g_authorConstructor = nullptr;
jmethodID g_categoryConstructor = nullptr;
jclass g_promoClass = nullptr;
jfieldID g_promoInstanceField = nullptr;
jmethodID g_onGalleryReceived = nullptr;
jmethodID g_onErrorReceived = nullptr;
uint64_t g_lastRequestId = 0;

void PrepareClassRefs(JNIEnv * env)
{
  if (g_galleryClass != nullptr)
    return;

  g_galleryClass = jni::GetGlobalClassRef(env, "com/mapswithme/maps/promo/PromoCityGallery");
  g_galleryConstructor =
      jni::GetConstructorID(env, g_galleryClass,
                            "([Lcom/mapswithme/maps/promo/PromoCityGallery$Item;"
                            "Ljava/lang/String;)V");
  g_itemClass = jni::GetGlobalClassRef(env, "com/mapswithme/maps/promo/PromoCityGallery$Item");
  g_itemConstructor =
      jni::GetConstructorID(env, g_itemClass,
                            "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;"
                            "Ljava/lang/String;Ljava/lang/String;"
                            "Lcom/mapswithme/maps/promo/PromoCityGallery$Author;"
                            "Lcom/mapswithme/maps/promo/PromoCityGallery$LuxCategory;)V");
  g_authorClass =
      jni::GetGlobalClassRef(env, "com/mapswithme/maps/promo/PromoCityGallery$Author");
  g_authorConstructor =
      jni::GetConstructorID(env, g_authorClass, "(Ljava/lang/String;Ljava/lang/String;)V");
  g_categoryClass =
      jni::GetGlobalClassRef(env, "com/mapswithme/maps/promo/PromoCityGallery$LuxCategory");
  g_categoryConstructor =
      jni::GetConstructorID(env, g_authorClass, "(Ljava/lang/String;Ljava/lang/String;)V");

  g_promoClass = jni::GetGlobalClassRef(env, "com/mapswithme/maps/promo/Promo");
  g_promoInstanceField =
      jni::GetStaticFieldID(env, g_promoClass, "INSTANCE", "Lcom/mapswithme/maps/promo/Promo;");
  jobject promoInstance = env->GetStaticObjectField(g_promoClass, g_promoInstanceField);
  g_onGalleryReceived = jni::GetMethodID(env, promoInstance, "onCityGalleryReceived",
                                         "(Lcom/mapswithme/maps/promo/PromoCityGallery;)V");
  g_onErrorReceived = jni::GetMethodID(env, promoInstance, "onErrorReceived", "()V");
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
    jni::TScopedLocalRef authorId(env, jni::ToJavaString(env, item.m_author.m_id));
    jni::TScopedLocalRef authorName(env, jni::ToJavaString(env, item.m_author.m_name));
    jni::TScopedLocalRef luxCategoryName(env, jni::ToJavaString(env, item.m_luxCategory.m_name));
    jni::TScopedLocalRef luxCategoryColor(env, jni::ToJavaString(env, item.m_luxCategory.m_color));

    jni::TScopedLocalRef author(
        env, env->NewObject(g_authorClass, g_authorConstructor, authorId.get(), authorName.get()));
    jni::TScopedLocalRef luxCategory(
        env, env->NewObject(g_categoryClass, g_categoryConstructor, luxCategoryName.get(),
                            luxCategoryColor.get()));

    return env->NewObject(g_itemClass, g_itemConstructor, name.get(), url.get(), imageUrl.get(),
                          access.get(), tier.get(), author.get(), luxCategory.get());
  };

  jni::TScopedLocalObjectArrayRef items(env, jni::ToJavaArray(env, g_itemClass, gallery.m_items,
                                        itemBuilder));
  jni::TScopedLocalRef moreUrl(env, jni::ToJavaString(env, gallery.m_moreUrl));

  return env->NewObject(g_galleryClass, g_galleryConstructor, items.get(), moreUrl.get());
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
