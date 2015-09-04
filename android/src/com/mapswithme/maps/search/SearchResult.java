package com.mapswithme.maps.search;

/**
 * Class instances are created from native code.
 */
@SuppressWarnings("unused")
public class SearchResult
{
  public static final int TYPE_SUGGEST = 0;
  public static final int TYPE_RESULT = 1;

  public static class Description
  {
    public final String featureType;
    public final String region;
    public final String distance;
    public final String cuisine;
    public final int stars;
    public final boolean closedNow;

    public Description(String featureType, String region, String distance, String cuisine, int stars, boolean closedNow)
    {
      this.featureType = featureType;
      this.region = region;
      this.distance = distance;
      this.cuisine = cuisine;
      this.stars = stars;
      this.closedNow = closedNow;
    }
  }


  public final String name;
  public final String suggestion;

  public final int type;
  public final Description description;

  // Consecutive pairs of indexes (each pair contains : start index, length), specifying highlighted matches of original query in result
  public final int[] highlightRanges;

  public SearchResult(String name, String suggestion, int[] highlightRanges)
  {
    this.name = name;
    this.suggestion = suggestion;
    description = null;
    type = TYPE_SUGGEST;

    this.highlightRanges = highlightRanges;
  }

  public SearchResult(String name, Description description, int[] highlightRanges)
  {
    this.name = name;
    suggestion = null;
    type = TYPE_RESULT;
    this.description = description;
    this.highlightRanges = highlightRanges;
  }
}