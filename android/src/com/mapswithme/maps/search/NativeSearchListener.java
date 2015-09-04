package com.mapswithme.maps.search;

/**
 * Native search will return results via this interface.
 */
@SuppressWarnings("unused")
public interface NativeSearchListener
{
  void onResultsUpdate(int count, int queryId);

  void onResultsEnd(int queryId);
}
