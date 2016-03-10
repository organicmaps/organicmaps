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

jobject ToJavaResult(Result & result, bool hasPosition, double lat, double lon)
{
  JNIEnv * env = jni::GetEnv();

  jni::TScopedLocalIntArrayRef ranges(env, env->NewIntArray(result.GetHighlightRangesCount() * 2));
  jint * rawArr = env->GetIntArrayElements(ranges.get(), nullptr);
  for (int i = 0; i < result.GetHighlightRangesCount(); i++)
  {
    auto const & range = result.GetHighlightRange(i);
    rawArr[2 * i] = range.first;
    rawArr[2 * i + 1] = range.second;
  }
  env->ReleaseIntArrayElements(ranges.get(), rawArr, 0);

  ms::LatLon ll = ms::LatLon::Zero();
  string distance;
  if (result.HasPoint())
  {
    ll = MercatorBounds::ToLatLon(result.GetFeatureCenter());
    if (hasPosition)
    {
      double dummy;
      (void) g_framework->NativeFramework()->GetDistanceAndAzimut(result.GetFeatureCenter(), lat, lon, 0, distance, dummy);
    }
  }

  if (result.IsSuggest())
  {
    jni::TScopedLocalRef name(env, jni::ToJavaString(env, result.GetString()));
    jni::TScopedLocalRef suggest(env, jni::ToJavaString(env, result.GetSuggestionString()));
    jobject ret = env->NewObject(g_resultClass, g_suggestConstructor, name.get(), suggest.get(), ll.lat, ll.lon, ranges.get());
    ASSERT(ret, ());
    return ret;
  }

  search::AddressInfo info;
  if (result.GetResultType() == Result::RESULT_FEATURE)
  {
    ::Framework * frm = g_framework->NativeFramework();
    frm->LoadSearchResultMetadata(result);
    frm->GetSearchResultAddress(result);
  }

  jni::TScopedLocalRef featureType(env, jni::ToJavaString(env, result.GetFeatureType()));
  jni::TScopedLocalRef region(env, jni::ToJavaString(env, info.FormatAddress(search::AddressInfo::SEARCH_RESULT)));
  jni::TScopedLocalRef dist(env, jni::ToJavaString(env, distance));
  jni::TScopedLocalRef cuisine(env, jni::ToJavaString(env, result.GetCuisine()));
  jni::TScopedLocalRef desc(env, env->NewObject(g_descriptionClass, g_descriptionConstructor,
                                                featureType.get(), region.get(),
                                                dist.get(), cuisine.get(),
                                                result.GetStarsCount(),
                                                static_cast<jint>(result.IsOpenNow())));

  jni::TScopedLocalRef name(env, jni::ToJavaString(env, result.GetString()));
  jobject ret = env->NewObject(g_resultClass, g_resultConstructor, name.get(), desc.get(), ll.lat, ll.lon, ranges.get());
  ASSERT(ret, ());

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
}  // namespace

extern "C"
{
  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_search_SearchEngine_nativeInit(JNIEnv * env, jobject thiz)
  {
    if ( g_javaListener )
      env->DeleteGlobalRef(g_javaListener);
    g_javaListener = env->NewGlobalRef(thiz);
    g_updateResultsId = jni::GetMethodID(env, g_javaListener, "onResultsUpdate", "([Lcom/mapswithme/maps/search/SearchResult;J)V");
    g_endResultsId = jni::GetMethodID(env, g_javaListener, "onResultsEnd", "(J)V");
    g_resultClass = static_cast<jclass>(env->NewGlobalRef(env->FindClass("com/mapswithme/maps/search/SearchResult")));
    g_resultConstructor = jni::GetConstructorID(env, g_resultClass, "(Ljava/lang/String;Lcom/mapswithme/maps/search/SearchResult$Description;DD[I)V");
    g_suggestConstructor = jni::GetConstructorID(env, g_resultClass, "(Ljava/lang/String;Ljava/lang/String;DD[I)V");
    g_descriptionClass = static_cast<jclass>(env->NewGlobalRef(env->FindClass("com/mapswithme/maps/search/SearchResult$Description")));
    g_descriptionConstructor = jni::GetConstructorID(env, g_descriptionClass, "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;II)V");
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
    params.m_onResults = bind(&OnResults, _1, timestamp, false, hasPosition, lat, lon);

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
      params.m_onResults = bind(&OnResults, _1, timestamp, isMapAndTable, false /* hasPosition */, 0.0, 0.0);
      if (g_framework->NativeFramework()->Search(params))
        g_queryTimestamp = timestamp;
    }
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_search_SearchEngine_nativeRunSearchMaps(JNIEnv * env, jclass clazz, jbyteArray bytes, jstring lang, jlong timestamp)
  {
    search::SearchParams params;
    params.m_query = jni::ToNativeString(env, bytes);
    params.SetInputLocale(ReplaceDeprecatedLanguageCode(jni::ToNativeString(env, lang)));
    params.SetForceSearch(true);
    params.SetMode(search::Mode::World);
    params.SetSuggestsEnabled(false);
    params.m_onResults = bind(&OnResults, _1, timestamp, false /* isMapAndTable */, false /* hasPosition */, 0.0, 0.0);

    g_framework->NativeFramework()->Search(params);
    g_queryTimestamp = timestamp;
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
      g_framework->NativeFramework()->ShowSearchResults(results);
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
