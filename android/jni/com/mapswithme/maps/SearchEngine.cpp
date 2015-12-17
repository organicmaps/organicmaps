#include "Framework.hpp"

#include "base/thread.hpp"
#include "search/result.hpp"
#include "std/atomic.hpp"
#include "std/mutex.hpp"

#include "../core/jni_helper.hpp"
#include "../platform/Language.hpp"
#include "../platform/Platform.hpp"

using search::Results;
using search::Result;

namespace
{
// TODO yunitsky
// Do not cache search results here, after new search will be implemented.
// Currently we cannot serialize FeatureID of search result properly.
// Cache is needed to show results on the map after click in the list of results.
Results g_results;
mutex g_resultsMutex;
// Timestamp of last search query. Results with older stamps are ignored.
atomic<long long> g_queryTimestamp;
// Implements 'NativeSearchListener' java interface.
jobject g_javaListener;
jmethodID g_updateResultsId;
jmethodID g_endResultsId;
// Cached classes and methods to return results.
jclass g_resultClass;
jmethodID g_resultConstructor;
jmethodID g_suggestConstructor;
jclass g_descriptionClass;
jmethodID g_descriptionConstructor;

jobject ToJavaResult(Result result, bool hasPosition, double lat, double lon)
{
  JNIEnv * env = jni::GetEnv();

  jintArray ranges = env->NewIntArray(result.GetHighlightRangesCount() * 2);
  jint * rawArr = env->GetIntArrayElements(ranges, nullptr);
  for (int i = 0; i < result.GetHighlightRangesCount(); i++)
  {
    auto const & range = result.GetHighlightRange(i);
    rawArr[2 * i] = range.first;
    rawArr[2 * i + 1] = range.second;
  }
  env->ReleaseIntArrayElements(ranges, rawArr, 0);

  if (result.IsSuggest())
  {
    jstring name = jni::ToJavaString(env, result.GetString());
    jstring suggest = jni::ToJavaString(env, result.GetSuggestionString());
    jobject ret = env->NewObject(g_resultClass, g_suggestConstructor, name, suggest, ranges);
    ASSERT(ret, ());
    env->DeleteLocalRef(name);
    env->DeleteLocalRef(suggest);
    env->DeleteLocalRef(ranges);
    return ret;
  }

  string distance;
  if (hasPosition)
  {
    double dummy;
    (void) g_framework->NativeFramework()->GetDistanceAndAzimut(result.GetFeatureCenter(), lat, lon, 0, distance, dummy);
  }

  g_framework->NativeFramework()->LoadSearchResultMetadata(result);

  jstring featureType = jni::ToJavaString(env, result.GetFeatureType());
  jstring region = jni::ToJavaString(env, result.GetRegionString());
  jstring dist = jni::ToJavaString(env, distance.c_str());
  jstring cuisine = jni::ToJavaString(env, result.GetCuisine());
  jobject desc = env->NewObject(g_descriptionClass, g_descriptionConstructor,
                                featureType, region,
                                dist, cuisine,
                                result.GetStarsCount(),
                                result.IsClosed());
  ASSERT(desc, ());
  env->DeleteLocalRef(featureType);
  env->DeleteLocalRef(region);
  env->DeleteLocalRef(dist);
  env->DeleteLocalRef(cuisine);

  jstring name = jni::ToJavaString(env, result.GetString());

  double const poiLat = MercatorBounds::YToLat(result.GetFeatureCenter().y);
  double const poiLon = MercatorBounds::XToLon(result.GetFeatureCenter().x);
  jobject ret = env->NewObject(g_resultClass, g_resultConstructor, name, desc, poiLat, poiLon, ranges);
  ASSERT(ret, ());
  env->DeleteLocalRef(name);
  env->DeleteLocalRef(desc);
  env->DeleteLocalRef(ranges);

  return ret;
}

jobjectArray BuildJavaResults(Results const & results, bool hasPosition, double lat, double lon)
{
  JNIEnv * env = jni::GetEnv();
  lock_guard<mutex> guard(g_resultsMutex);
  g_results = results;

  int const count = g_results.GetCount();
  jobjectArray const jResults = env->NewObjectArray(count, g_resultClass, 0);
  for (int i = 0; i < count; i++)
  {
    jobject jRes = ToJavaResult(g_results.GetResult(i), hasPosition, lat, lon);
    env->SetObjectArrayElement(jResults, i, jRes);
    env->DeleteLocalRef(jRes);
  }
  return jResults;
}

void OnResults(Results const & results, long long timestamp, bool isMapAndTable,
               bool hasPosition, double lat, double lon)
{
  // Ignore results from obsolete searches.
  if (g_queryTimestamp > timestamp)
    return;

  JNIEnv * env = jni::GetEnv();

  if (results.IsEndMarker())
  {
    env->CallVoidMethod(g_javaListener, g_endResultsId, static_cast<jlong>(timestamp));
    if (isMapAndTable && results.IsEndedNormal())
      g_framework->NativeFramework()->UpdateUserViewportChanged();
    return;
  }

  jobjectArray const & jResults = BuildJavaResults(results, hasPosition, lat, lon);
  env->CallVoidMethod(g_javaListener, g_updateResultsId, jResults, static_cast<jlong>(timestamp));
  env->DeleteLocalRef(jResults);
}
} // namespace

extern "C"
{
  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_search_SearchEngine_nativeInit(JNIEnv * env, jobject thiz)
  {
    if ( g_javaListener )
      env->DeleteGlobalRef(g_javaListener);
    g_javaListener = env->NewGlobalRef(thiz);
    g_updateResultsId = jni::GetJavaMethodID(env, g_javaListener, "onResultsUpdate", "([Lcom/mapswithme/maps/search/SearchResult;J)V");
    ASSERT(g_updateResultsId, ());
    g_endResultsId = jni::GetJavaMethodID(env, g_javaListener, "onResultsEnd", "(J)V");
    ASSERT(g_endResultsId, ());
    g_resultClass = static_cast<jclass>(env->NewGlobalRef(env->FindClass("com/mapswithme/maps/search/SearchResult")));
    ASSERT(g_resultClass, ());
    g_resultConstructor = env->GetMethodID(g_resultClass, "<init>", "(Ljava/lang/String;Lcom/mapswithme/maps/search/SearchResult$Description;DD[I)V");
    ASSERT(g_resultConstructor, ());
    g_suggestConstructor = env->GetMethodID(g_resultClass, "<init>", "(Ljava/lang/String;Ljava/lang/String;[I)V");
    ASSERT(g_suggestConstructor, ());
    g_descriptionClass = static_cast<jclass>(env->NewGlobalRef(env->FindClass("com/mapswithme/maps/search/SearchResult$Description")));
    ASSERT(g_descriptionClass, ());
    g_descriptionConstructor = env->GetMethodID(g_descriptionClass, "<init>", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;IZ)V");
    ASSERT(g_descriptionConstructor, ());
  }

  JNIEXPORT jboolean JNICALL
  Java_com_mapswithme_maps_search_SearchEngine_nativeRunSearch(JNIEnv * env, jclass clazz, jbyteArray bytes, jstring lang,
                                                               jlong timestamp, jboolean force, jboolean hasPosition, jdouble lat, jdouble lon)
  {
    search::SearchParams params;
    params.m_query = jni::ToNativeString(env, bytes);
    params.SetInputLocale(ReplaceDeprecatedLanguageCode(jni::ToNativeString(env, lang)));
    params.SetForceSearch(force);
    if (hasPosition)
      params.SetPosition(lat, lon);
    params.m_callback = bind(&OnResults, _1, timestamp, false, hasPosition, lat, lon);

    bool const searchStarted = g_framework->NativeFramework()->Search(params);
    if (searchStarted)
      g_queryTimestamp = timestamp;
    return searchStarted;
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_search_SearchEngine_nativeRunInteractiveSearch(JNIEnv * env, jclass clazz, jbyteArray bytes,
                                                                          jstring lang, jlong timestamp, jboolean isMapAndTable)
  {
    search::SearchParams params;
    params.m_query = jni::ToNativeString(env, bytes);
    params.SetInputLocale(ReplaceDeprecatedLanguageCode(jni::ToNativeString(env, lang)));

    g_framework->NativeFramework()->StartInteractiveSearch(params);

    if (isMapAndTable)
    {
      params.m_callback = bind(&OnResults, _1, timestamp, isMapAndTable,
                               false /* hasPosition */, 0, 0);
      if (g_framework->NativeFramework()->Search(params))
        g_queryTimestamp = timestamp;
    }
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_search_SearchEngine_nativeShowResult(JNIEnv * env, jclass clazz, jint index)
  {
    lock_guard<mutex> guard(g_resultsMutex);
    Result const & result = g_results.GetResult(index);
    g_framework->PostDrapeTask([result]()
    {
      g_framework->NativeFramework()->ShowSearchResult(result);
    });
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_search_SearchEngine_nativeShowAllResults(JNIEnv * env, jclass clazz)
  {
    lock_guard<mutex> guard(g_resultsMutex);
    auto const & results = g_results;
    g_framework->PostDrapeTask([results]()
    {
      g_framework->NativeFramework()->ShowAllSearchResults(results);
    });
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_search_SearchEngine_nativeCancelInteractiveSearch(JNIEnv * env, jclass clazz)
  {
    GetPlatform().RunOnGuiThread([]()
    {
      g_framework->NativeFramework()->CancelInteractiveSearch();
    });
  }
} // extern "C"
