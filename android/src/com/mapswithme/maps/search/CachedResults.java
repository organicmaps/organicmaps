package com.mapswithme.maps.search;

import android.util.SparseArray;

/**
 * Helper to avoid unneccessary invocations of native functions to obtain search result objects.
 */
class CachedResults
{
  private final SearchFragment mFragment;
  private final SparseArray<SearchResult> mCache = new SparseArray<>();
  private int mCurrentQueryId = -1;

  public CachedResults(SearchFragment fragment)
  {
    mFragment = fragment;
  }

  public SearchResult get(int position, int queryId)
  {
    SearchResult res = null;
    if (queryId == mCurrentQueryId)
      res = mCache.get(position);
    else
    {
      mCache.clear();
      mCurrentQueryId = queryId;
    }

    if (res == null)
    {
      res = mFragment.getUncachedResult(position, queryId);
      mCache.put(position, res);
    }
    return res;
  }
}
