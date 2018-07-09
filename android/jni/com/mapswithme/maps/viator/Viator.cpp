#include "com/mapswithme/maps/viator/Viator.hpp"
#include "com/mapswithme/maps/Framework.hpp"

#include <string>

namespace
{
jclass g_viatorClass = nullptr;
jclass g_viatorProductClass;
jmethodID g_viatorProductConstructor;
jmethodID g_viatorCallback;
std::string g_lastDestId;

void PrepareClassRefs(JNIEnv * env)
{
  if (g_viatorClass != nullptr)
    return;

  g_viatorClass = jni::GetGlobalClassRef(env, "com/mapswithme/maps/viator/Viator");
  g_viatorProductClass = jni::GetGlobalClassRef(env, "com/mapswithme/maps/viator/ViatorProduct");
  g_viatorProductConstructor =
      jni::GetConstructorID(env, g_viatorProductClass,
                            "(Ljava/lang/String;DILjava/lang/String;DLjava/lang/String;Ljava/lang/"
                            "String;Ljava/lang/String;Ljava/lang/String;)V");
  g_viatorCallback =
      jni::GetStaticMethodID(env, g_viatorClass, "onViatorProductsReceived",
                             "(Ljava/lang/String;[Lcom/mapswithme/maps/viator/ViatorProduct;)V");
}
}  // namespace

jobjectArray ToViatorProductsArray(std::vector<viator::Product> const & products)
{
  JNIEnv * env = jni::GetEnv();
  PrepareClassRefs(env);

  return jni::ToJavaArray(env, g_viatorProductClass, products,
                          [](JNIEnv * env, viator::Product const & item)
    {
      jni::TScopedLocalRef jTitle(env, jni::ToJavaString(env, item.m_title));
      jni::TScopedLocalRef jDuration(env, jni::ToJavaString(env, item.m_duration));
      jni::TScopedLocalRef jPriceFormatted(env, jni::ToJavaString(env, item.m_priceFormatted));
      jni::TScopedLocalRef jCurrency(env, jni::ToJavaString(env, item.m_currency));
      jni::TScopedLocalRef jPhotoUrl(env, jni::ToJavaString(env, item.m_photoUrl));
      jni::TScopedLocalRef jPageUrl(env, jni::ToJavaString(env, item.m_pageUrl));
      return env->NewObject(g_viatorProductClass, g_viatorProductConstructor, jTitle.get(),
                            item.m_rating, item.m_reviewCount, jDuration.get(), item.m_price,
                            jPriceFormatted.get(), jCurrency.get(), jPhotoUrl.get(),
                            jPageUrl.get());
    });
}

void OnViatorProductsReceived(std::string const & destId,
                              std::vector<viator::Product> const & products)
{
  if (g_lastDestId != destId)
    return;

  JNIEnv * env = jni::GetEnv();
  jni::TScopedLocalRef jDestId(env, jni::ToJavaString(env, destId));
  jni::TScopedLocalObjectArrayRef jProducts(env, ToViatorProductsArray(products));
  env->CallStaticVoidMethod(g_viatorClass, g_viatorCallback, jDestId.get(), jProducts.get());
}

extern "C" {

JNIEXPORT void JNICALL Java_com_mapswithme_maps_viator_Viator_nativeRequestViatorProducts(
    JNIEnv * env, jclass clazz, jobject policy, jstring destId, jstring currency)
{
  PrepareClassRefs(env);

  g_lastDestId = jni::ToNativeString(env, destId);
  g_framework->RequestViatorProducts(env, policy, g_lastDestId, jni::ToNativeString(env, currency),
                                     &OnViatorProductsReceived);
}
}  // extern "C"
