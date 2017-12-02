package com.mapswithme.maps.viator;

import android.os.Parcel;
import android.os.Parcelable;
import android.support.annotation.NonNull;

public final class ViatorProduct implements Parcelable
{
  @NonNull
  private final String mTitle;
  private final double mRating;
  private final int mReviewCount;
  @NonNull
  private final String mDuration;
  private final double mPrice;
  @NonNull
  private final String mPriceFormatted;
  @NonNull
  private final String mCurrency;
  @NonNull
  private final String mPhotoUrl;
  @NonNull
  private final String mPageUrl;

  public static final Creator<ViatorProduct> CREATOR = new Creator<ViatorProduct>()
  {
    @Override
    public ViatorProduct createFromParcel(Parcel in)
    {
      return new ViatorProduct(in);
    }

    @Override
    public ViatorProduct[] newArray(int size)
    {
      return new ViatorProduct[size];
    }
  };

  @SuppressWarnings("unused")
  public ViatorProduct(@NonNull String title, double rating, int reviewCount,
                       @NonNull String duration, double price, @NonNull String priceFormatted,
                       @NonNull String currency, @NonNull String photoUrl, @NonNull String pageUrl)
  {
    mTitle = title;
    mRating = rating;
    mReviewCount = reviewCount;
    mDuration = duration;
    mPrice = price;
    mPriceFormatted = priceFormatted;
    mCurrency = currency;
    mPhotoUrl = photoUrl;
    mPageUrl = pageUrl;
  }

  private ViatorProduct(Parcel in)
  {
    mTitle = in.readString();
    mRating = in.readDouble();
    mReviewCount = in.readInt();
    mDuration = in.readString();
    mPrice = in.readDouble();
    mPriceFormatted = in.readString();
    mCurrency = in.readString();
    mPhotoUrl = in.readString();
    mPageUrl = in.readString();
  }

  @Override
  public void writeToParcel(Parcel dest, int flags)
  {
    dest.writeString(mTitle);
    dest.writeDouble(mRating);
    dest.writeInt(mReviewCount);
    dest.writeString(mDuration);
    dest.writeDouble(mPrice);
    dest.writeString(mPriceFormatted);
    dest.writeString(mCurrency);
    dest.writeString(mPhotoUrl);
    dest.writeString(mPageUrl);
  }

  @Override
  public int describeContents()
  {
    return 0;
  }

  @NonNull
  public String getTitle()
  {
    return mTitle;
  }

  public double getRating()
  {
    return mRating;
  }

  public int getReviewCount()
  {
    return mReviewCount;
  }

  @NonNull
  public String getDuration()
  {
    return mDuration;
  }

  public double getPrice()
  {
    return mPrice;
  }

  @NonNull
  public String getPriceFormatted()
  {
    return mPriceFormatted;
  }

  @NonNull
  public String getCurrency()
  {
    return mCurrency;
  }

  @NonNull
  public String getPhotoUrl()
  {
    return mPhotoUrl;
  }

  @NonNull
  public String getPageUrl()
  {
    return mPageUrl;
  }

  @Override
  public boolean equals(Object o)
  {
    if (this == o) return true;
    if (o == null || getClass() != o.getClass()) return false;

    ViatorProduct that = (ViatorProduct) o;

    if (Double.compare(that.mRating, mRating) != 0) return false;
    if (mReviewCount != that.mReviewCount) return false;
    if (Double.compare(that.mPrice, mPrice) != 0) return false;
    if (!mTitle.equals(that.mTitle)) return false;
    if (!mDuration.equals(that.mDuration)) return false;
    if (!mPriceFormatted.equals(that.mPriceFormatted)) return false;
    if (!mCurrency.equals(that.mCurrency)) return false;
    if (!mPhotoUrl.equals(that.mPhotoUrl)) return false;
    return mPageUrl.equals(that.mPageUrl);
  }

  @Override
  public int hashCode()
  {
    int result;
    long temp;
    result = mTitle.hashCode();
    temp = Double.doubleToLongBits(mRating);
    result = 31 * result + (int) (temp ^ (temp >>> 32));
    result = 31 * result + mReviewCount;
    result = 31 * result + mDuration.hashCode();
    temp = Double.doubleToLongBits(mPrice);
    result = 31 * result + (int) (temp ^ (temp >>> 32));
    result = 31 * result + mPriceFormatted.hashCode();
    result = 31 * result + mCurrency.hashCode();
    result = 31 * result + mPhotoUrl.hashCode();
    result = 31 * result + mPageUrl.hashCode();
    return result;
  }
}
