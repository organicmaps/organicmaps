package com.mapswithme.maps.gallery;

import android.content.Context;
import android.os.Parcel;
import android.os.Parcelable;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import androidx.annotation.StringRes;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.FeatureId;
import com.mapswithme.maps.search.Popularity;
import com.mapswithme.maps.search.SearchResult;
import com.mapswithme.util.UiUtils;

import static com.mapswithme.maps.gallery.Constants.TYPE_MORE;
import static com.mapswithme.maps.gallery.Constants.TYPE_PRODUCT;
import static com.mapswithme.util.Constants.Rating.RATING_INCORRECT_VALUE;

public class Items
{
  public static class LocalExpertItem extends RegularAdapterStrategy.Item
  {
    @Nullable
    private final String mPhotoUrl;
    private final double mPrice;
    @NonNull
    private final String mCurrency;
    private final double mRating;

    public LocalExpertItem(@Constants.ViewType int viewType, @StringRes int titleId,
                           @Nullable String url, @Nullable String photoUrl, double price,
                           @NonNull String currency, double rating)
    {
      super(viewType, titleId, null, url);
      mPhotoUrl = photoUrl;
      mPrice = price;
      mCurrency = currency;
      mRating = rating;
    }

    public LocalExpertItem(@Constants.ViewType int viewType, @Nullable String title,
                           @Nullable String url, @Nullable String photoUrl, double price,
                           @NonNull String currency, double rating)
    {
      super(viewType, title, null, url);
      mPhotoUrl = photoUrl;
      mPrice = price;
      mCurrency = currency;
      mRating = rating;
    }

    @Nullable
    String getPhotoUrl()
    {
      return mPhotoUrl;
    }

    public double getPrice()
    {
      return mPrice;
    }

    @NonNull
    public String getCurrency()
    {
      return mCurrency;
    }

    public double getRating()
    {
      return mRating;
    }
  }

  public static class LocalExpertMoreItem extends LocalExpertItem
  {

    public LocalExpertMoreItem(@Nullable String url)
    {
      super(TYPE_MORE, R.string.placepage_more_button, url,
            null, 0, "", 0);
    }
  }

  public static class SearchItem extends RegularAdapterStrategy.Item
  {
    @NonNull
    private final SearchResult mResult;

    public SearchItem(@NonNull SearchResult result)
    {
      super(TYPE_PRODUCT, result.name, result.description.featureType, null);
      mResult = result;
    }

    public SearchItem(@NonNull String title)
    {
      super(TYPE_MORE, title, null, null);
      mResult = SearchResult.EMPTY;
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

    public int getStars()
    {
      if (mResult.description == null)
        return 0;

      return mResult.description.stars;
    }

    public float getRating()
    {
      if (mResult.description == null)
        return RATING_INCORRECT_VALUE;

      return mResult.description.rating;
    }

    @Nullable
    public String getPrice()
    {
      if (mResult.description == null)
        return null;

      return mResult.description.pricing;
    }

    @Nullable
    public String getFeatureType()
    {
      if (mResult.description == null)
        return null;

      return mResult.description.featureType;
    }

    @NonNull
    public FeatureId getFeatureId()
    {
      if (mResult.description == null)
        return FeatureId.EMPTY;

      return mResult.description.featureId == null ? FeatureId.EMPTY : mResult.description.featureId;
    }

    @NonNull
    public Popularity getPopularity()
    {
      return mResult.getPopularity();
    }
  }

  public static class MoreSearchItem extends SearchItem
  {
    public MoreSearchItem(@NonNull Context context)
    {
      super(context.getString(R.string.placepage_more_button));
    }
  }

  public static class Item implements Parcelable
  {
    @StringRes
    private final int mTitleId;
    @Nullable
    private final String mTitle;
    @Nullable
    private final String mUrl;
    @Nullable
    private final String mSubtitle;

    public Item(@StringRes int titleId, @Nullable String url,
                @Nullable String subtitle)
    {
      mTitleId = titleId;
      mTitle = null;
      mUrl = url;
      mSubtitle = subtitle;
    }

    public Item(@Nullable String title, @Nullable String url,
                @Nullable String subtitle)
    {
      mTitleId = UiUtils.NO_ID;
      mTitle = title;
      mUrl = url;
      mSubtitle = subtitle;
    }
    protected Item(Parcel in)
    {
      mTitleId = in.readInt();
      mTitle = in.readString();
      mUrl = in.readString();
      mSubtitle = in.readString();
    }

    public static final Creator<Item> CREATOR = new Creator<Item>()
    {
      @Override
      public Item createFromParcel(Parcel in)
      {
        return new Item(in);
      }

      @Override
      public Item[] newArray(int size)
      {
        return new Item[size];
      }
    };

    @Nullable
    public String getTitle(@NonNull Context context)
    {
      if(mTitle == null)
        return context.getString(mTitleId);
      else
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

    @Override
    public int describeContents()
    {
      return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags)
    {
      dest.writeInt(mTitleId);
      dest.writeString(mTitle);
      dest.writeString(mUrl);
      dest.writeString(mSubtitle);
    }
  }
}
