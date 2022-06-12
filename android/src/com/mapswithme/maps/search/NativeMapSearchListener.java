package com.mapswithme.maps.search;

import androidx.annotation.Keep;

public interface NativeMapSearchListener
{
  @Keep
  class Result
  {
    public final String countryId;
    public final String matchedString;

    public Result(String countryId, String matchedString)
    {
      this.countryId = countryId;
      this.matchedString = matchedString;
    }
  }

  void onMapSearchResults(Result[] results, long timestamp, boolean isLast);
}
