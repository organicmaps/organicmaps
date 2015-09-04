#include "Framework.hpp"

#include "base/thread.hpp"
#include "search/result.hpp"

#include "../core/jni_helper.hpp"
#include "../platform/Language.hpp"

class SearchAdapter
{
  /// @name Results holder. Store last valid results from search threads (m_storeID)
  /// and current result to show in GUI (m_ID).
  //@{
  search::Results m_storeResults, m_results;
  int m_storeID, m_ID;
  //@}

  threads::Mutex m_updateMutex;

  /// Last saved fragment to run update UI.
  jobject m_fragment;

  // This function may be called several times for one queryID.
  // In that case we should increment m_storeID to distinguish different results.
  // Main queryID is incremented by 5-step to leave space for middle queries.
  // This constant should be equal with SearchFragment.QUERY_STEP;
  static int const QUERY_STEP = 5;

  void OnResults(search::Results const & res, int queryID)
  {
    if (m_fragment == 0)
    {
      // In case when fragment is destroyed, but search thread passed any results.
      return;
    }

    JNIEnv * env = jni::GetEnv();

    if (res.IsEndMarker())
    {
      jmethodID const methodId = jni::GetJavaMethodID(env, m_fragment, "onResultsEnd", "(I)V");
      env->CallVoidMethod(m_fragment, methodId, queryID);
      return;
    }

    // IMPORTANT! mutex must be unlocked before updateData Java call at the bottom of the method, so that consecutive usage of native methods
    // from java didnt cause deadlocks. so MutexGuard creation & usage should be wrapped by braces.
    {
      threads::MutexGuard guard(m_updateMutex);

      // store current results
      m_storeResults = res;

      if (m_storeID >= queryID && m_storeID < queryID + QUERY_STEP)
      {
        ++m_storeID;
        // not more than QUERY_STEP results per query
        ASSERT_LESS ( m_storeID, queryID + QUERY_STEP, () );
      }
      else
      {
        ASSERT_LESS ( m_storeID, queryID, () );
        m_storeID = queryID;
      }
    }

    // post message to update ListView in UI thread
    jmethodID const id = jni::GetJavaMethodID(env, m_fragment, "onResultsUpdate", "(II)V");
    env->CallVoidMethod(m_fragment, id,
                          static_cast<jint>(m_storeResults.GetCount()),
                          static_cast<jint>(m_storeID));
  }

  bool AcquireShowResults(int resultID)
  {
    if (resultID != m_ID)
    {
      {
        // Grab last results.
        threads::MutexGuard guard(m_updateMutex);
        if (m_ID != m_storeID)
        {
          m_results.Swap(m_storeResults);
          m_ID = m_storeID;
        }
      }

      if (resultID != m_ID)
      {
        // It happens when acquiring obsolete results.
        return false;
      }
    }

    return true;
  }

  bool CheckPosition(int position) const
  {
    return (position < static_cast<int>(m_results.GetCount()));
  }

  SearchAdapter()
    : m_ID(0), m_storeID(0), m_fragment(0)
  {
  }

  void Connect(JNIEnv * env, jobject fragment)
  {
    threads::MutexGuard guard(m_updateMutex);

    if (m_fragment != 0)
      env->DeleteGlobalRef(m_fragment);
    m_fragment = env->NewGlobalRef(fragment);

    m_storeID = m_ID = 0;
  }

  void Disconnect(JNIEnv * env)
  {
    if (m_fragment != 0)
    {
      threads::MutexGuard guard(m_updateMutex);

      env->DeleteGlobalRef(m_fragment);
      m_fragment = 0;
    }
  }

  static SearchAdapter * s_pInstance;

public:
  /// @name Instance lifetime functions.
  //@{
  static void ConnectInstance(JNIEnv * env, jobject fragment)
  {
    if (s_pInstance == 0)
      s_pInstance = new SearchAdapter();

    s_pInstance->Connect(env, fragment);
  }

  static void DisconnectInstance(JNIEnv * env)
  {
    if (s_pInstance)
      s_pInstance->Disconnect(env);
  }

  static SearchAdapter & Instance()
  {
    ASSERT ( s_pInstance, () );
    return *s_pInstance;
  }
  //@}

  bool RunSearch(JNIEnv * env, search::SearchParams & params, int queryID)
  {
    params.m_callback = bind(&SearchAdapter::OnResults, this, _1, queryID);

    return g_framework->Search(params);
  }

  void ShowItem(int position)
  {
    if (CheckPosition(position))
      g_framework->ShowSearchResult(m_results.GetResult(position));
    else
      LOG(LERROR, ("Invalid position", position));
  }

  void ShowAllResults()
  {
    if (m_results.GetCount() > 0)
      g_framework->ShowAllSearchResults();
    else
      LOG(LERROR, ("There is no results to show."));
  }

  search::Result const * GetResult(int position, int resultID)
  {
    if (AcquireShowResults(resultID) && CheckPosition(position))
      return &(m_results.GetResult(position));
    return 0;
  }
};

SearchAdapter * SearchAdapter::s_pInstance = 0;

extern "C"
{

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_search_SearchFragment_nativeConnectSearchListener(JNIEnv * env, jobject thiz)
{
  SearchAdapter::ConnectInstance(env, thiz);
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_search_SearchFragment_nativeDisconnectSearchListener(JNIEnv * env, jobject thiz)
{
  SearchAdapter::DisconnectInstance(env);
}

JNIEXPORT jboolean JNICALL
Java_com_mapswithme_maps_search_SearchFragment_nativeRunSearch(
    JNIEnv * env, jobject thiz, jstring s, jstring lang,
    jint queryID, jboolean force, jboolean hasPosition, jdouble lat, jdouble lon)
{
  search::SearchParams params;

  params.m_query = jni::ToNativeString(env, s);
  params.SetInputLocale(ReplaceDeprecatedLanguageCode(jni::ToNativeString(env, lang)));
  params.SetForceSearch(force);
  if (hasPosition)
    params.SetPosition(lat, lon);

  return SearchAdapter::Instance().RunSearch(env, params, queryID);
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_search_SearchFragment_nativeShowItem(JNIEnv * env, jobject thiz, jint position)
{
  SearchAdapter::Instance().ShowItem(position);
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_search_SearchFragment_nativeShowAllSearchResults(JNIEnv * env, jclass clazz)
{
  SearchAdapter::Instance().ShowAllResults();
}

JNIEXPORT jobject JNICALL
Java_com_mapswithme_maps_search_SearchFragment_nativeGetResult(
    JNIEnv * env, jobject thiz, jint position, jint queryID,
    jboolean hasPosition, jdouble lat, jdouble lon, jdouble north)
{
  search::Result const * res = SearchAdapter::Instance().GetResult(position, queryID);
  if (res == nullptr) return 0;

  jintArray ranges = env->NewIntArray(res->GetHighlightRangesCount() * 2);
  jint * narr = env->GetIntArrayElements(ranges, NULL);
  for (int i = 0, j = 0; i < res->GetHighlightRangesCount(); ++i)
  {
    pair<uint16_t, uint16_t> const & range = res->GetHighlightRange(i);
    narr[j++] = range.first;
    narr[j++] = range.second;
  }

  env->ReleaseIntArrayElements(ranges, narr, 0);

  static shared_ptr<jobject> resultClassGlobalRef = jni::make_global_ref(env->FindClass("com/mapswithme/maps/search/SearchResult"));
  jclass resultClass = static_cast<jclass>(*resultClassGlobalRef.get());
  ASSERT(resultClass, ());

  if (res->IsSuggest())
  {
    static jmethodID suggestCtor = env->GetMethodID(resultClass, "<init>", "(;Ljava/lang/String;;Ljava/lang/String;[I)V");
    ASSERT(suggestCtor, ());

    return env->NewObject(resultClass, suggestCtor,
                          jni::ToJavaString(env, res->GetString()),
                          jni::ToJavaString(env, res->GetSuggestionString()),
                          static_cast<jintArray>(ranges));
  }

  static jmethodID resultCtor = env->GetMethodID(klass, "<init>",
            "(Ljava/lang/String;Lcom/mapswithme/maps/search/SearchResult$Description;[I)V");
  ASSERT ( resultCtor, () );

  string distance;
  if (hasPosition)
  {
    double dummy;
    (void) g_framework->NativeFramework()->GetDistanceAndAzimut(res->GetFeatureCenter(), lat, lon, north, distance, dummy);
  }

  static shared_ptr<jobject> descClassGlobalRef = jni::make_global_ref(env->FindClass("com/mapswithme/maps/search/SearchResult$Description"));
  jclass descClass = static_cast<jclass>(*descClassGlobalRef.get());
  ASSERT(descClass, ());

  static jmethodID descCtor = env->GetMethodID(descClass, "<init>", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;IZ)V");
  ASSERT(descCtor, ());

  jobject desc = env->NewObject(descClass, descCtor,
                                jni::ToJavaString(env, res->GetFeatureType()),
                                jni::ToJavaString(env, res->GetRegionString()),
                                jni::ToJavaString(env, distance.c_str()),
                                jni::ToJavaString(env, res->GetCuisine()),
                                res->GetStarsCount(),
                                res->IsClosed());

  return env->NewObject(resultClass, resultCtor,
                        jni::ToJavaString(env, res->GetString()),
                        desc,
                        static_cast<jintArray>(ranges));
}

JNIEXPORT jstring JNICALL
Java_com_mapswithme_maps_search_SearchFragment_nativeGetLastQuery(JNIEnv * env, jobject thiz)
{
  return jni::ToJavaString(env, g_framework->GetLastSearchQuery());
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_search_SearchFragment_nativeClearLastQuery(JNIEnv * env, jobject thiz)
{
  g_framework->ClearLastSearchQuery();
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_search_SearchFragment_runInteractiveSearch(JNIEnv * env, jobject thiz,
                              jstring query, jstring lang)
{
  search::SearchParams params;
  params.m_query = jni::ToNativeString(env, query);
  params.SetInputLocale(ReplaceDeprecatedLanguageCode(jni::ToNativeString(env, lang)));

  g_framework->NativeFramework()->StartInteractiveSearch(params);
}

}
