package com.mapswithme.maps.search;

import androidx.annotation.NonNull;

import com.mapswithme.maps.bookmarks.data.FeatureId;

/**
 * Class instances are created from native code.
 */
@SuppressWarnings("unused")
public class SearchResult implements PopularityProvider
{
  public static final int TYPE_SUGGEST = 0;
  public static final int TYPE_RESULT = 1;

  // Values should match osm::YesNoUnknown enum.
  public static final int OPEN_NOW_UNKNOWN = 0;
  public static final int OPEN_NOW_YES = 1;
  public static final int OPEN_NOW_NO = 2;

  public static final SearchResult EMPTY = new SearchResult("", "", 0, 0,
                                                            new int[] {});

  public static class Description
  {
    public final FeatureId featureId;
    public final String featureType;
    public final String region;
    public final String distance;
    public final String cuisine;
    public final String brand;
    public final String airportIata;
    public final String roadShields;
    public final int openNow;
    public final int minutesUntilOpen;
    public final int minutesUntilClosed;
    public final boolean hasPopularityHigherPriority;

    public Description(FeatureId featureId, String featureType, String region, String distance,
                       String cuisine, String brand, String airportIata, String roadShields,
                       int openNow, int minutesUntilOpen, int minutesUntilClosed,
                       boolean hasPopularityHigherPriority)
    {
      this.featureId = featureId;
      this.featureType = featureType;
      this.region = region;
      this.distance = distance;
      this.cuisine = cuisine;
      this.brand = brand;
      this.airportIata = airportIata;
      this.roadShields = roadShields;
      this.openNow = openNow;
      this.minutesUntilOpen = minutesUntilOpen;
      this.minutesUntilClosed = minutesUntilClosed;
      this.hasPopularityHigherPriority = hasPopularityHigherPriority;
    }
  }

  public final String name;
  public final String suggestion;
  public final double lat;
  public final double lon;

  public final int type;
  public final Description description;

  // Consecutive pairs of indexes (each pair contains : start index, length), specifying highlighted matches of original query in result
  public final int[] highlightRanges;

  public final int stars;
  public final boolean isHotel;
  @NonNull
  private final Popularity mPopularity;

  public SearchResult(String name, String suggestion, double lat, double lon, int[] highlightRanges)
  {
    this.name = name;
    this.suggestion = suggestion;
    this.lat = lat;
    this.lon = lon;
    this.stars = 0;
    this.isHotel = false;
    this.description = null;
    this.type = TYPE_SUGGEST;
    this.highlightRanges = highlightRanges;
    mPopularity = Popularity.defaultInstance();
  }

  public SearchResult(String name, Description description, double lat, double lon, int[] highlightRanges,
                      boolean isHotel, int stars, @NonNull Popularity popularity)
  {
    this.type = TYPE_RESULT;
    this.name = name;
    this.stars = stars;
    this.isHotel = isHotel;
    mPopularity = popularity;
    this.suggestion = null;
    this.lat = lat;
    this.lon = lon;
    this.description = description;
    this.highlightRanges = highlightRanges;
  }

  @NonNull
  @Override
  public Popularity getPopularity()
  {
    return mPopularity;
  }
}
