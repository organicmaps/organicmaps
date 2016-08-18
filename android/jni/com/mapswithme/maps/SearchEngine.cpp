#include "Framework.hpp"

#include "search/everywhere_search_params.hpp"
#include "search/mode.hpp"
#include "search/result.hpp"
#include "search/viewport_search_params.hpp"

#include "base/thread.hpp"

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

// Implements 'NativeMapSearchListener' java interface.
jmethodID g_mapResultsMethod;
jclass g_mapResultClass;
jmethodID g_mapResultCtor;

jobject ToJavaResult(Result & result, bool hasPosition, double lat, double lon)
{
  JNIEnv * env = jni::GetEnv();
  ::Framework * fr = g_framework->NativeFramework();

  jni::TScopedLocalIntArrayRef ranges(env, env->NewIntArray(result.GetHighlightRangesCount() * 2));
  jint * rawArr = env->GetIntArrayElements(ranges, nullptr);
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
      (void) fr->GetDistanceAndAzimut(result.GetFeatureCenter(), lat, lon, 0, distance, dummy);
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

  jni::TScopedLocalRef featureType(env, jni::ToJavaString(env, result.GetFeatureType()));
  jni::TScopedLocalRef address(env, jni::ToJavaString(env, result.GetAddress()));
  jni::TScopedLocalRef dist(env, jni::ToJavaString(env, distance));
  jni::TScopedLocalRef cuisine(env, jni::ToJavaString(env, result.GetCuisine()));
  jni::TScopedLocalRef rating(env, jni::ToJavaString(env, result.GetHotelRating()));
  jni::TScopedLocalRef pricing(env, jni::ToJavaString(env, result.GetHotelApproximatePricing()));
  jni::TScopedLocalRef desc(env, env->NewObject(g_descriptionClass, g_descriptionConstructor,
                                                featureType.get(), address.get(),
                                                dist.get(), cuisine.get(),
                                                rating.get(), pricing.get(),
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
  jobjectArray const jResults = env->NewObjectArray(count, g_resultClass, nullptr);
  for (int i = 0; i < count; i++)
  {
    jni::TScopedLocalRef jRes(env, ToJavaResult(g_results.GetResult(i), hasPosition, lat, lon));
    env->SetObjectArrayElement(jResults, i, jRes.get());
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

  jni::TScopedLocalObjectArrayRef jResults(env, BuildJavaResults(results, hasPosition, lat, lon));
  env->CallVoidMethod(g_javaListener, g_updateResultsId, jResults.get(), static_cast<jlong>(timestamp));
}

jobjectArray BuildJavaMapResults(vector<storage::DownloaderSearchResult> const & results)
{
  JNIEnv * env = jni::GetEnv();
  lock_guard<mutex> guard(g_resultsMutex);

  int const count = results.size();
  jobjectArray const res = env->NewObjectArray(count, g_mapResultClass, nullptr);
  for (int i = 0; i < count; i++)
  {
    jni::TScopedLocalRef country(env, jni::ToJavaString(env, results[i].m_countryId));
    jni::TScopedLocalRef matched(env, jni::ToJavaString(env, results[i].m_matchedName));
    jni::TScopedLocalRef item(env, env->NewObject(g_mapResultClass, g_mapResultCtor, country.get(), matched.get()));
    env->SetObjectArrayElement(res, i, item.get());
  }

  return res;
}

void OnMapSearchResults(storage::DownloaderSearchResults const & results, long long timestamp)
{
  // Ignore results from obsolete searches.
  if (g_queryTimestamp > timestamp)
    return;

  JNIEnv * env = jni::GetEnv();
  jni::TScopedLocalObjectArrayRef jResults(env, BuildJavaMapResults(results.m_results));
  env->CallVoidMethod(g_javaListener, g_mapResultsMethod, jResults.get(), static_cast<jlong>(timestamp), results.m_endMarker);
}

}  // namespace

extern "C"
{
  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_search_SearchEngine_nativeInit(JNIEnv * env, jobject thiz)
  {
    g_javaListener = env->NewGlobalRef(thiz);
    g_updateResultsId = jni::GetMethodID(env, g_javaListener, "onResultsUpdate", "([Lcom/mapswithme/maps/search/SearchResult;J)V");
    g_endResultsId = jni::GetMethodID(env, g_javaListener, "onResultsEnd", "(J)V");
    g_resultClass = jni::GetGlobalClassRef(env, "com/mapswithme/maps/search/SearchResult");
    g_resultConstructor = jni::GetConstructorID(env, g_resultClass, "(Ljava/lang/String;Lcom/mapswithme/maps/search/SearchResult$Description;DD[I)V");
    g_suggestConstructor = jni::GetConstructorID(env, g_resultClass, "(Ljava/lang/String;Ljava/lang/String;DD[I)V");
    g_descriptionClass = jni::GetGlobalClassRef(env, "com/mapswithme/maps/search/SearchResult$Description");
    g_descriptionConstructor = jni::GetConstructorID(env, g_descriptionClass, "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;II)V");

    g_mapResultsMethod = jni::GetMethodID(env, g_javaListener, "onMapSearchResults", "([Lcom/mapswithme/maps/search/NativeMapSearchListener$Result;JZ)V");
    g_mapResultClass = jni::GetGlobalClassRef(env, "com/mapswithme/maps/search/NativeMapSearchListener$Result");
    g_mapResultCtor = jni::GetConstructorID(env, g_mapResultClass, "(Ljava/lang/String;Ljava/lang/String;)V");
  }

  JNIEXPORT jboolean JNICALL
  Java_com_mapswithme_maps_search_SearchEngine_nativeRunSearch(JNIEnv * env, jclass clazz, jbyteArray bytes, jstring lang,
                                                               jlong timestamp, jboolean hasPosition, jdouble lat, jdouble lon)
  {
    search::EverywhereSearchParams params;
    params.m_query = jni::ToNativeString(env, bytes);
    params.m_inputLocale = ReplaceDeprecatedLanguageCode(jni::ToNativeString(env, lang));
    params.m_onResults = bind(&OnResults, _1, timestamp, false, hasPosition, lat, lon);

    bool const searchStarted = g_framework->NativeFramework()->SearchEverywhere(params);
    if (searchStarted)
      g_queryTimestamp = timestamp;
    return searchStarted;
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_search_SearchEngine_nativeRunInteractiveSearch(JNIEnv * env, jclass clazz, jbyteArray bytes,
                                                                          jstring lang, jlong timestamp, jboolean isMapAndTable)
  {
    search::ViewportSearchParams vparams;
    vparams.m_query = jni::ToNativeString(env, bytes);
    vparams.m_inputLocale = ReplaceDeprecatedLanguageCode(jni::ToNativeString(env, lang));

    g_framework->NativeFramework()->SearchInViewport(vparams);

    if (isMapAndTable)
    {
      search::EverywhereSearchParams eparams;
      eparams.m_query = vparams.m_query;
      eparams.m_inputLocale = vparams.m_inputLocale;
      eparams.m_onResults = bind(&OnResults, _1, timestamp, isMapAndTable, false /* hasPosition */,
                                 0.0 /* lat */, 0.0 /* lon */);
      if (g_framework->NativeFramework()->SearchEverywhere(eparams))
        g_queryTimestamp = timestamp;
    }
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_search_SearchEngine_nativeRunSearchMaps(JNIEnv * env, jclass clazz, jbyteArray bytes, jstring lang, jlong timestamp)
  {
    storage::DownloaderSearchParams params;
    params.m_query = jni::ToNativeString(env, bytes);
    params.m_inputLocale = ReplaceDeprecatedLanguageCode(jni::ToNativeString(env, lang));
    params.m_onResults = bind(&OnMapSearchResults, _1, timestamp);

    if (g_framework->NativeFramework()->SearchInDownloader(params))
      g_queryTimestamp = timestamp;
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_search_SearchEngine_nativeShowResult(JNIEnv * env, jclass clazz, jint index)
  {
    lock_guard<mutex> guard(g_resultsMutex);
    Result const & result = g_results.GetResult(index);
    g_framework->NativeFramework()->ShowSearchResult(result);
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_search_SearchEngine_nativeShowAllResults(JNIEnv * env, jclass clazz)
  {
    lock_guard<mutex> guard(g_resultsMutex);
    g_framework->NativeFramework()->ShowSearchResults(g_results);
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_search_SearchEngine_nativeCancelInteractiveSearch(JNIEnv * env, jclass clazz)
  {
    GetPlatform().RunOnGuiThread([]()
    {
      g_framework->NativeFramework()->CancelSearch(search::Mode::Viewport);
    });
  }
} // extern "C"
