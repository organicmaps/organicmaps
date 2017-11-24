#include "com/mapswithme/core/jni_helper.hpp"
#include "com/mapswithme/maps/discovery/Locals.hpp"
#include "com/mapswithme/maps/viator/Viator.hpp"
#include "com/mapswithme/maps/Framework.hpp"
#include "com/mapswithme/maps/SearchEngine.hpp"

#include "map/discovery/discovery_manager.hpp"

#include "geometry/mercator.hpp"

#include "search/result.hpp"

#include <memory>
#include <utility>
#include <vector>

using namespace std::placeholders;

namespace
{
jclass g_discoveryManagerClass = nullptr;
jfieldID g_discoveryManagerInstanceField;
jmethodID g_onResultReceivedMethod;
jmethodID g_onViatorProductsReceivedMethod;
jmethodID g_onLocalExpertsReceivedMethod;
jmethodID g_onErrorMethod;
uint32_t g_lastRequestId = 0;

void PrepareClassRefs(JNIEnv * env)
{
  if (g_discoveryManagerClass != nullptr)
    return;

  g_discoveryManagerClass = jni::GetGlobalClassRef(env, "com/mapswithme/maps/discovery/DiscoveryManager");

  g_discoveryManagerInstanceField = jni::GetStaticFieldID(env, g_discoveryManagerClass, "INSTANCE",
                                                          "Lcom/mapswithme/maps/discovery/DiscoveryManager;");

  jobject discoveryManagerInstance = env->GetStaticObjectField(g_discoveryManagerClass,
                                                               g_discoveryManagerInstanceField);

  g_onResultReceivedMethod = jni::GetMethodID(env, discoveryManagerInstance, "onResultReceived",
                                              "([Lcom/mapswithme/maps/search/SearchResult;I)V");

  g_onViatorProductsReceivedMethod = jni::GetMethodID(env, discoveryManagerInstance,
                                                      "onViatorProductsReceived",
                                                      "([Lcom/mapswithme/maps/viator/ViatorProduct;)V");

  g_onLocalExpertsReceivedMethod = jni::GetMethodID(env, discoveryManagerInstance,
                                                    "onLocalExpertsReceived",
                                                    "([Lcom/mapswithme/maps/discovery/LocalExpert;)V");

  g_onErrorMethod = jni::GetMethodID(env, discoveryManagerInstance, "onError", "(I)V");
}

struct DiscoveryCallback
{
  void operator()(uint32_t const requestId, search::Results const & results,
                  discovery::ItemType const type, m2::PointD const & viewportCenter) const
  {
    if (g_lastRequestId != requestId)
      return;

    ASSERT(g_discoveryManagerClass != nullptr, ());
    JNIEnv * env = jni::GetEnv();

    auto const lat = MercatorBounds::YToLat(viewportCenter.y);
    auto const lon = MercatorBounds::XToLon(viewportCenter.x);
    std::vector<bool> customers(results.GetCount(), false);
    jni::TScopedLocalObjectArrayRef jResults(env, BuildSearchResults(results, customers,
                                                                     true /* hasPosition */,
                                                                     lat, lon));
    jobject discoveryManagerInstance = env->GetStaticObjectField(g_discoveryManagerClass,
                                                                 g_discoveryManagerInstanceField);
    env->CallVoidMethod(discoveryManagerInstance, g_onResultReceivedMethod,
                        jResults.get(), static_cast<jint>(type));

    jni::HandleJavaException(env);
  }

  void operator()(uint32_t const requestId, std::vector<viator::Product> const & products) const
  {
    if (g_lastRequestId != requestId)
      return;

    ASSERT(g_discoveryManagerClass != nullptr, ());
    JNIEnv * env = jni::GetEnv();

    jni::TScopedLocalObjectArrayRef jProducts(env, ToViatorProductsArray(products));
    jobject discoveryManagerInstance = env->GetStaticObjectField(g_discoveryManagerClass,
                                                                 g_discoveryManagerInstanceField);
    env->CallVoidMethod(discoveryManagerInstance, g_onViatorProductsReceivedMethod,
                        jProducts.get());

    jni::HandleJavaException(env);
  }

  void operator()(uint32_t const requestId, std::vector<locals::LocalExpert> const & experts) const
  {
    if (g_lastRequestId != requestId)
      return;

    ASSERT(g_discoveryManagerClass != nullptr, ());
    JNIEnv * env = jni::GetEnv();

    jni::TScopedLocalObjectArrayRef jLocals(env, ToLocalExpertsArray(experts));
    jobject discoveryManagerInstance = env->GetStaticObjectField(g_discoveryManagerClass,
                                                                 g_discoveryManagerInstanceField);
    env->CallVoidMethod(discoveryManagerInstance, g_onLocalExpertsReceivedMethod, jLocals.get());

    jni::HandleJavaException(env);
  }
};

void OnDiscoveryError(uint32_t const requestId, discovery::ItemType const type)
{
  if (g_lastRequestId != requestId)
    return;

  ASSERT(g_discoveryManagerClass != nullptr, ());

  JNIEnv * env = jni::GetEnv();
  jobject discoveryManagerInstance = env->GetStaticObjectField(g_discoveryManagerClass,
                                                               g_discoveryManagerInstanceField);
  env->CallVoidMethod(discoveryManagerInstance, g_onErrorMethod, static_cast<jint>(type));

  jni::HandleJavaException(env);
}
}  // namespace

extern "C" {

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_discovery_DiscoveryManager_nativeDiscover(JNIEnv * env, jclass,
                                                                   jobject params)
{
  PrepareClassRefs(env);

  discovery::ClientParams p;
  auto const paramsClass = env->GetObjectClass(params);
  static auto const currencyField = env->GetFieldID(paramsClass, "mCurrency", "Ljava/lang/String;");
  {
    auto const currency = static_cast<jstring>(env->GetObjectField(params, currencyField));
    string const res = jni::ToNativeString(env, currency);
    if (!res.empty())
      p.m_currency = res;
  }

  static auto const langField = env->GetFieldID(paramsClass, "mLang", "Ljava/lang/String;");
  {
    auto const lang = static_cast<jstring>(env->GetObjectField(params, langField));
    string const res = jni::ToNativeString(env, lang);
    if (!res.empty())
      p.m_lang = res;
  }

  static auto const itemsCountField = env->GetFieldID(paramsClass, "mItemsCount", "I");
  {
    auto const count = env->GetIntField(params, itemsCountField);
    ASSERT_GREATER(count, 0, ());
    p.m_itemsCount = static_cast<size_t>(count);
  }

  static auto const itemTypesField = env->GetFieldID(paramsClass, "mItemTypes", "[I");
  {
    auto const array = static_cast<jintArray>(env->GetObjectField(params, itemTypesField));
    auto const length = env->GetArrayLength(array);
    ASSERT_GREATER(length, 0, ());

    auto const dtor = [array, env](jint * data) { env->ReleaseIntArrayElements(array, data, 0); };
    std::unique_ptr<jint, decltype(dtor)> data{env->GetIntArrayElements(array, nullptr), dtor};

    std::vector<discovery::ItemType> itemTypes;
    for (jsize i = 0; i < length; ++i)
      itemTypes.emplace_back(static_cast<discovery::ItemType>(data.get()[i]));

    p.m_itemTypes = std::move(itemTypes);
  }

  g_lastRequestId = g_framework->NativeFramework()->Discover(std::move(p), DiscoveryCallback(),
                                                             std::bind(&OnDiscoveryError, _1, _2));
}
}  // extern "C"
