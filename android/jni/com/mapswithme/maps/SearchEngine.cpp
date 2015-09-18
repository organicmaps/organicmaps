#include "Framework.hpp"

#include "base/thread.hpp"
#include "search/result.hpp"

#include "../core/jni_helper.hpp"
#include "../platform/Language.hpp"
#include "../platform/Platform.hpp"

namespace
{
  ::Framework * frm() { return g_framework->NativeFramework(); }
}

class SearchEngine
{
  /// @name Search results are stored in the holder and returned to Java with request.
  //@{
  search::Results m_results;
  //@}
  // Timestamp of last search query. Results with older stamps are ignored.
  std::atomic_llong m_queryTimestamp;
  threads::Mutex m_updateMutex;
  // Implements 'NativeSearchListener' java interface.
  shared_ptr<jobject> m_listenerPtr;

  void OnResults(search::Results const & results, long long timestamp)
  {
    // Ignore results from obsolete searches.
    if (m_queryTimestamp > timestamp)
      return;

    JNIEnv * env = jni::GetEnv();

    jobject listener = *m_listenerPtr.get();
    if (results.IsEndMarker())
    {
      static jmethodID const resultId = jni::GetJavaMethodID(env, listener, "onResultsEnd", "(J)V");
      env->CallVoidMethod(listener, resultId, static_cast<jlong>(timestamp));
      return;
    }

    SaveResults(results);
    ShowResults();

    static jmethodID const updateId = jni::GetJavaMethodID(env, listener, "onResultsUpdate", "(IJ)V");
    env->CallVoidMethod(listener, updateId,
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
    android::Platform::RunOnGuiThreadImpl(bind(&SearchEngine::ShowResultsImpl, this));
  }

  void ShowResultsImpl()
  {
    frm()->UpdateSearchResults(m_results);
  }

  SearchEngine() : m_queryTimestamp(0)
  {
  }

public:
  static SearchEngine & Instance()
  {
    static SearchEngine instance;
    return instance;
  }

  void ConnectListener(JNIEnv *env, jobject listener)
  {
    m_listenerPtr = jni::make_global_ref(listener);
  }

  bool RunSearch(JNIEnv * env, search::SearchParams & params, long long timestamp)
  {
    params.m_callback = bind(&SearchEngine::OnResults, this, _1, timestamp);
    bool const searchStarted = frm()->Search(params);
    if (searchStarted)
      m_queryTimestamp = timestamp;

    return searchStarted;
  }

  void RunInteractiveSearch(JNIEnv * env, search::SearchParams & params, long long timestamp)
  {
    params.m_callback = bind(&SearchEngine::OnResults, this, _1, timestamp);
    frm()->StartInteractiveSearch(params);
    frm()->UpdateUserViewportChanged();
    m_queryTimestamp = timestamp;
  }

  void ShowResult(int position) const
  {
    if (IsIndexValid(position))
      g_framework->ShowSearchResult(m_results.GetResult(position));
    else
      LOG(LERROR, ("Invalid position", position));
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
    SearchEngine::Instance().ConnectListener(env, thiz);
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

    return SearchEngine::Instance().RunSearch(env, params, timestamp);
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_search_SearchEngine_nativeShowResult(JNIEnv * env, jobject thiz, jint index)
  {
    SearchEngine::Instance().ShowResult(index);
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_search_SearchEngine_nativeShowAllResults(JNIEnv * env, jclass clazz)
  {
    SearchEngine::Instance().ShowAllResults();
  }

  JNIEXPORT jobject JNICALL
  Java_com_mapswithme_maps_search_SearchEngine_nativeGetResult(
      JNIEnv * env, jobject thiz, jint index, jlong timestamp,
      jboolean hasPosition, jdouble lat, jdouble lon)
  {
    if (!SearchEngine::Instance().IsIndexValid(index))
      return nullptr;

    search::Result & res = SearchEngine::Instance().GetResult(index);

    bool isSuggest = res.IsSuggest();
    if (!isSuggest)
      frm()->LoadSearchResultMetadata(res);

    jintArray ranges = env->NewIntArray(res.GetHighlightRangesCount() * 2);
    jint * narr = env->GetIntArrayElements(ranges, NULL);
    for (int i = 0, j = 0; i < res.GetHighlightRangesCount(); ++i)
    {
      pair<uint16_t, uint16_t> const & range = res.GetHighlightRange(i);
      narr[j++] = range.first;
      narr[j++] = range.second;
    }

    env->ReleaseIntArrayElements(ranges, narr, 0);

    static shared_ptr<jobject> resultClassGlobalRef = jni::make_global_ref(env->FindClass("com/mapswithme/maps/search/SearchResult"));
    jclass resultClass = static_cast<jclass>(*resultClassGlobalRef.get());
    ASSERT(resultClass, ());

    if (isSuggest)
    {
      static jmethodID suggestCtor = env->GetMethodID(resultClass, "<init>", "(Ljava/lang/String;Ljava/lang/String;[I)V");
      ASSERT(suggestCtor, ());

      return env->NewObject(resultClass, suggestCtor,
                            jni::ToJavaString(env, res.GetString()),
                            jni::ToJavaString(env, res.GetSuggestionString()),
                            ranges);
    }

    static jmethodID resultCtor = env->GetMethodID(resultClass, "<init>",
                                                   "(Ljava/lang/String;Lcom/mapswithme/maps/search/SearchResult$Description;[I)V");
    ASSERT(resultCtor, ());

    string distance;
    if (hasPosition)
    {
      double dummy;
      (void) frm()->GetDistanceAndAzimut(res.GetFeatureCenter(), lat, lon, 0, distance, dummy);
    }

    static shared_ptr<jobject> descClassGlobalRef = jni::make_global_ref(env->FindClass("com/mapswithme/maps/search/SearchResult$Description"));
    jclass descClass = static_cast<jclass>(*descClassGlobalRef.get());
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

    return env->NewObject(resultClass, resultCtor,
                          jni::ToJavaString(env, res.GetString()),
                          desc,
                          ranges);
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_search_SearchEngine_nativeRunInteractiveSearch(JNIEnv * env, jobject thiz, jstring query, jstring lang, jlong timestamp)
  {
    search::SearchParams params;
    params.m_query = jni::ToNativeString(env, query);
    params.SetInputLocale(ReplaceDeprecatedLanguageCode(jni::ToNativeString(env, lang)));
    SearchEngine::Instance().RunInteractiveSearch(env, params, timestamp);
  }
} // extern "C"
