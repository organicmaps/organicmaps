package com.mapswithme.maps.widget.placepage;

import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.maps.bookmarks.data.Metadata;

@SuppressWarnings("WeakerAccess")
public final class SponsoredHotelInfo
{
  private String mId;

  final String rating;
  final String price;
  final String urlBook;
  final String urlDescription;

  public SponsoredHotelInfo(String rating, String price, String urlBook, String urlDescription)
  {
    this.rating = rating;
    this.price = price;
    this.urlBook = urlBook;
    this.urlDescription = urlDescription;
  }

  public void updateId(MapObject point)
  {
    mId = point.getMetadata(Metadata.MetadataType.FMD_SPONSORED_ID);
  }

  public String getId()
  {
    return mId;
  }
}
