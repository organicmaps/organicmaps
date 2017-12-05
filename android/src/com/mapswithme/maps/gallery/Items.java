package com.mapswithme.maps.gallery;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.search.SearchResult;

import static com.mapswithme.maps.gallery.Constants.TYPE_MORE;
import static com.mapswithme.maps.gallery.Constants.TYPE_PRODUCT;

public class Items
{
  public static class ViatorItem extends RegularAdapterStrategy.Item
  {
    @Nullable
    final String mPhotoUrl;
    @Nullable
    final String mDuration;
    final double mRating;
    @Nullable
    final String mPrice;

    public ViatorItem(@Nullable String photoUrl, @NonNull String title,
                      @Nullable String duration, double rating, @Nullable String price,
                      @NonNull String url)
    {
      super(TYPE_PRODUCT, title, url, null);
      mPhotoUrl = photoUrl;
      mDuration = duration;
      mRating = rating;
      mPrice = price;
    }

    ViatorItem(@Constants.ViewType int type, @NonNull String title, @Nullable String url)
    {
      super(type, title, url, null);
      mPhotoUrl = null;
      mDuration = null;
      mRating = -1;
      mPrice = null;
    }
  }

  public static class ViatorMoreItem extends ViatorItem
  {

    public ViatorMoreItem(@Nullable String url)
    {
      super(TYPE_MORE, MwmApplication.get().getString(R.string.placepage_more_button), url);
    }
  }

  public static class CianItem extends RegularAdapterStrategy.Item
  {
    @NonNull
    final String mPrice;
    @NonNull
    final String mAddress;

    public CianItem(@NonNull String title, @NonNull String url, @NonNull String price,
                    @NonNull String address)
    {
      super(TYPE_PRODUCT, title, url, null);
      mPrice = price;
      mAddress = address;
    }

    CianItem(@Constants.ViewType int type, @NonNull String title, @Nullable String url)
    {
      super(type, title, url, null);
      mPrice = "";
      mAddress = "";
    }
  }

  public static class SearchItem extends RegularAdapterStrategy.Item
  {
    @NonNull
    private final SearchResult mResult;

    public SearchItem(@NonNull SearchResult result)
    {
      super(TYPE_PRODUCT, result.name, null, result.description.featureType);
      mResult = result;
    }

    @NonNull
    public String getDistance()
    {
      SearchResult.Description d = mResult.description;
      return d != null ? d.distance : "";
    }

    public double getLat()
    {
      return mResult.lat;
    }

    public double getLon()
    {
      return mResult.lon;
    }
  }

  public static class CianMoreItem extends CianItem
  {

    public CianMoreItem(@Nullable String url)
    {
      super(TYPE_MORE, MwmApplication.get().getString(R.string.placepage_more_button), url);
    }
  }

  public static class Item
  {
    @NonNull
    private final String mTitle;
    @Nullable
    private final String mUrl;
    @Nullable
    private final String mSubtitle;

    public Item(@NonNull String title, @Nullable String url,
                @Nullable String subtitle)
    {
      mTitle = title;
      mUrl = url;
      mSubtitle = subtitle;
    }

    @NonNull
    public String getTitle()
    {
      return mTitle;
    }

    @Nullable
    public String getSubtitle()
    {
      return mSubtitle;
    }

    @Nullable
    public String getUrl()
    {
      return mUrl;
    }
  }
}
