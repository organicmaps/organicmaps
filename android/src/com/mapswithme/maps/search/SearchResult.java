package com.mapswithme.maps.search;

/**
 * Class instances are created from native code.
 */
public class SearchResult
{
  public String mName;
  public String mSuggestion;
  public String mCountry;
  public String mAmenity;
  public String mDistance;

  // 0 - suggestion result
  // 1 - feature result
  public static final int TYPE_SUGGESTION = 0;
  public static final int TYPE_FEATURE = 1;

  public int mType;
  // consecutive pairs of indexes (each pair contains : start index, length), specifying highlighted substrings of original query
  public int[] mHighlightRanges;

  public SearchResult(String name, String suggestion, int[] highlightRanges)
  {
    mName = name;
    mSuggestion = suggestion;
    mType = TYPE_SUGGESTION;
    mHighlightRanges = highlightRanges;
  }

  public SearchResult(String name, String country, String amenity,
                      String distance, int[] highlightRanges)
  {
    mName = name;
    mCountry = country;
    mAmenity = amenity;
    mDistance = distance;
    mType = TYPE_FEATURE;
    mHighlightRanges = highlightRanges;
  }
}