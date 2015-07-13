package com.mapswithme.maps.search;

/**
 * Native search will return results according this interface.
 */
@SuppressWarnings("unused")
public interface NativeSearchListener
{
  void onResultsUpdate(final int count, final int resultId);

  void onResultsEnd();
}
