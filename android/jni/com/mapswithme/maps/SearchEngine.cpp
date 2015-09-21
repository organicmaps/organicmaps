#include "Framework.hpp"

#include "base/thread.hpp"
#include "search/result.hpp"
#include "std/atomic.hpp"

#include "../core/jni_helper.hpp"
#include "../platform/Language.hpp"
#include "../platform/Platform.hpp"

class SearchResultsCache
{
  /// @name Search results are stored in the holder and returned to Java with request.
  //@{
  search::Results m_results;
  //@}
  // Timestamp of last search query. Results with older stamps are ignored.
  atomic<long long> m_queryTimestamp;
  threads::Mutex m_updateMutex;
  // Implements 'NativeSearchListener' java interface.
  jclass m_javaListener = nullptr;

  void OnResults(search::Results const & results, long long timestamp)
  {
    // Ignore results from obsolete searches.
    if (m_queryTimestamp > timestamp)
      return;

    JNIEnv * env = jni::GetEnv();

    if (results.IsEndMarker())
    {
      static jmethodID const resultId = jni::GetJavaMethodID(env, m_javaListener, "onResultsEnd", "(J)V");
      env->CallVoidMethod(m_javaListener, resultId, static_cast<jlong>(timestamp));
      return;
    }

    SaveResults(results);
    ShowResults();

    static jmethodID const updateId = jni::GetJavaMethodID(env, m_javaListener, "onResultsUpdate", "(IJ)V");
    env->CallVoidMethod(m_javaListener, updateId,
                        static_cast<jint>(m_results.GetCount()),
                        static_cast<jlong>(timestamp));
  }

  void SaveResults(search::Results const & results)
  {
    threads::MutexGuard guard(m_updateMutex);
    m_results = results;
  }

  void ShowResults()
  {
    android::Platform::RunOnGuiThreadImpl(bind(&SearchResultsCache::ShowResultsImpl, this));
  }

  void ShowResultsImpl()
  {
    g_framework->NativeFramework()->UpdateSearchResults(m_results);
  }

  SearchResultsCache() : m_queryTimestamp(0)
  {}

public:
  static SearchResultsCache & Instance()
  {
    static SearchResultsCache instance;
    return instance;
  }

  void ConnectListener(JNIEnv * env, jobject listener)
  {
    m_javaListener = static_cast<jclass>(env->NewGlobalRef(listener));
  }

  bool RunSearch(search::SearchParams & params, long long timestamp)
  {
    params.m_callback = bind(&SearchResultsCache::OnResults, this, _1, timestamp);
    bool const searchStarted = g_framework->NativeFramework()->Search(params);
    if (searchStarted)
      m_queryTimestamp = timestamp;
    return searchStarted;
  }

  void RunInteractiveSearch(search::SearchParams & params, long long timestamp)
  {
    params.m_callback = bind(&SearchResultsCache::OnResults, this, _1, timestamp);
    g_framework->NativeFramework()->StartInteractiveSearch(params);
    g_framework->NativeFramework()->UpdateUserViewportChanged();
    m_queryTimestamp = timestamp;
  }

  void ShowResult(int index) const
  {
    if (IsIndexValid(index))
      g_framework->ShowSearchResult(m_results.GetResult(index));
    else
      LOG(LERROR, ("Invalid index ", index));
  }

  void ShowAllResults() const
  {
    if (m_results.GetCount() > 0)
      g_framework->ShowAllSearchResults();
    else
      LOG(LERROR, ("There is no results to show."));
  }

  bool IsIndexValid(int index) const
  {
    return (index < static_cast<int>(m_results.GetCount()));
  }

  search::Result & GetResult(int index)
  {
    return m_results.GetResult(index);
  }
};

extern "C"
{
  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_search_SearchEngine_nativeInit(JNIEnv * env, jobject thiz)
  {
    SearchResultsCache::Instance().ConnectListener(env, thiz);
  }

  JNIEXPORT jboolean JNICALL
  Java_com_mapswithme_maps_search_SearchEngine_nativeRunSearch(
      JNIEnv * env, jobject thiz, jstring s, jstring lang,
      jlong timestamp, jboolean force, jboolean hasPosition, jdouble lat, jdouble lon)
  {
    search::SearchParams params;
    params.m_query = jni::ToNativeString(env, s);
    params.SetInputLocale(ReplaceDeprecatedLanguageCode(jni::ToNativeString(env, lang)));
    params.SetForceSearch(force);
    if (hasPosition)
      params.SetPosition(lat, lon);
    return SearchResultsCache::Instance().RunSearch(params, timestamp);
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_search_SearchEngine_nativeShowResult(JNIEnv * env, jobject thiz, jint index)
  {
    SearchResultsCache::Instance().ShowResult(index);
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_search_SearchEngine_nativeShowAllResults(JNIEnv * env, jclass clazz)
  {
    SearchResultsCache::Instance().ShowAllResults();
  }

  JNIEXPORT jobject JNICALL
  Java_com_mapswithme_maps_search_SearchEngine_nativeGetResult(
      JNIEnv * env, jobject thiz, jint index, jlong timestamp,
      jboolean hasPosition, jdouble lat, jdouble lon)
  {
    if (!SearchResultsCache::Instance().IsIndexValid(index))
      return nullptr;

    search::Result & res = SearchResultsCache::Instance().GetResult(index);

    bool const isSuggest = res.IsSuggest();
    if (!isSuggest)
      g_framework->NativeFramework()->LoadSearchResultMetadata(res);

    jintArray ranges = env->NewIntArray(res.GetHighlightRangesCount() * 2);
    jint * narr = env->GetIntArrayElements(ranges, NULL);
    for (int i = 0, j = 0; i < res.GetHighlightRangesCount(); ++i)
    {
      pair<uint16_t, uint16_t> const & range = res.GetHighlightRange(i);
      narr[j++] = range.first;
      narr[j++] = range.second;
    }

    env->ReleaseIntArrayElements(ranges, narr, 0);

    static jclass resultClass = static_cast<jclass>(env->NewGlobalRef(env->FindClass("com/mapswithme/maps/search/SearchResult")));
    ASSERT(resultClass, ());

    if (isSuggest)
    {
      static jmethodID suggestCtor = env->GetMethodID(resultClass, "<init>", "(Ljava/lang/String;Ljava/lang/String;[I)V");
      ASSERT(suggestCtor, ());

      jobject ret = env->NewObject(resultClass, suggestCtor,
                                   jni::ToJavaString(env, res.GetString()),
                                   jni::ToJavaString(env, res.GetSuggestionString()),
                                   ranges);
      ASSERT(ret, ());

      return ret;
    }

    static jmethodID resultCtor = env->GetMethodID(resultClass, "<init>",
                                                   "(Ljava/lang/String;Lcom/mapswithme/maps/search/SearchResult$Description;[I)V");
    ASSERT(resultCtor, ());

    string distance;
    if (hasPosition)
    {
      double dummy;
      (void) g_framework->NativeFramework()->GetDistanceAndAzimut(res.GetFeatureCenter(), lat, lon, 0, distance, dummy);
    }

    static jclass descClass = static_cast<jclass>(env->NewGlobalRef(env->FindClass("com/mapswithme/maps/search/SearchResult$Description")));
    ASSERT(descClass, ());

    static jmethodID descCtor = env->GetMethodID(descClass, "<init>", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;IZ)V");
    ASSERT(descCtor, ());

    jobject desc = env->NewObject(descClass, descCtor,
                                  jni::ToJavaString(env, res.GetFeatureType()),
                                  jni::ToJavaString(env, res.GetRegionString()),
                                  jni::ToJavaString(env, distance.c_str()),
                                  jni::ToJavaString(env, res.GetCuisine()),
                                  res.GetStarsCount(),
                                  res.IsClosed());
    ASSERT(desc, ());

    jobject ret = env->NewObject(resultClass, resultCtor,
                                 jni::ToJavaString(env, res.GetString()),
                                 desc,
                                 ranges);
    ASSERT(ret, ());

    return ret;
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_search_SearchEngine_nativeRunInteractiveSearch(JNIEnv * env, jobject thiz, jstring query, jstring lang, jlong timestamp)
  {
    search::SearchParams params;
    params.m_query = jni::ToNativeString(env, query);
    params.SetInputLocale(ReplaceDeprecatedLanguageCode(jni::ToNativeString(env, lang)));
    SearchResultsCache::Instance().RunInteractiveSearch(params, timestamp);
  }
} // extern "C"
