package com.mapswithme.maps.search;

import com.mapswithme.maps.bookmarks.data.FeatureId;

import static com.mapswithme.maps.search.SearchResultTypes.TYPE_LOCAL_ADS_CUSTOMER;
import static com.mapswithme.maps.search.SearchResultTypes.TYPE_RESULT;
import static com.mapswithme.maps.search.SearchResultTypes.TYPE_SUGGEST;

/**
 * Class instances are created from native code.
 */
@SuppressWarnings("unused")
public class SearchResult implements SearchData
{
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
    public final String rating;
    public final String pricing;
    public final int stars;
    public final int openNow;

    public Description(FeatureId featureId, String featureType, String region, String distance,
                       String cuisine, String rating, String pricing, int stars, int openNow)
    {
      this.featureId = featureId;
      this.featureType = featureType;
      this.region = region;
      this.distance = distance;
      this.cuisine = cuisine;
      this.rating = rating;
      this.pricing = pricing;
      this.stars = stars;
      this.openNow = openNow;
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

  public final boolean isHotel;

  public SearchResult(String name, String suggestion, double lat, double lon, int[] highlightRanges)
  {
    this.name = name;
    this.suggestion = suggestion;
    this.lat = lat;
    this.lon = lon;
    this.isHotel = false;
    this.description = null;
    this.type = TYPE_SUGGEST;

    this.highlightRanges = highlightRanges;
  }

  public SearchResult(String name, Description description, double lat, double lon, int[] highlightRanges,
                      boolean isHotel, boolean isLocalAdsCustomer)
  {
    this.type = isLocalAdsCustomer ? TYPE_LOCAL_ADS_CUSTOMER : TYPE_RESULT;
    this.name = name;
    this.isHotel = isHotel;
    this.suggestion = null;
    this.lat = lat;
    this.lon = lon;
    this.description = description;
    this.highlightRanges = highlightRanges;
  }

  @Override
  public int getItemViewType()
  {
    return type;
  }
}
