package com.mapswithme.maps.locals;

import android.os.Parcel;
import android.os.Parcelable;

import android.support.annotation.NonNull;

final class LocalExpert implements Parcelable
{
  private final int mId;
  @NonNull
  private final String mName;
  @NonNull
  private final String mCountry;
  @NonNull
  private final String mCity;
  private final double mRating;
  private final int mReviewCount;
  private final double mPrice;
  @NonNull
  private final String mCurrency;
  @NonNull
  private final String mMotto;
  @NonNull
  private final String mAboutExpert;
  @NonNull
  private final String mOfferDescription;
  @NonNull
  private final String mPageUrl;
  @NonNull
  private final String mPhotoUrl;


  public static final Creator<LocalExpert> CREATOR = new Creator<LocalExpert>()
  {
    @Override
    public LocalExpert createFromParcel(Parcel in)
    {
      return new LocalExpert(in);
    }

    @Override
    public LocalExpert[] newArray(int size)
    {
      return new LocalExpert[size];
    }
  };

  @SuppressWarnings("unused")
  public LocalExpert(int id, @NonNull String name, @NonNull String country,
                     @NonNull String city, double rating, int reviewCount,
                     double price, @NonNull String currency, @NonNull String motto,
                     @NonNull String about, @NonNull String offer, @NonNull String pageUrl,
                     @NonNull String photoUrl)
  {
    mId = id;
    mName = name;
    mCountry = country;
    mCity = city;
    mRating = rating;
    mReviewCount = reviewCount;
    mPrice = price;
    mCurrency = currency;
    mMotto = motto;
    mAboutExpert = about;
    mOfferDescription = offer;
    mPhotoUrl = photoUrl;
    mPageUrl = pageUrl;
  }

  private LocalExpert(Parcel in)
  {
    mId = in.readInt();
    mName = in.readString();
    mCountry = in.readString();
    mCity = in.readString();
    mRating = in.readDouble();
    mReviewCount = in.readInt();
    mPrice = in.readDouble();
    mCurrency = in.readString();
    mMotto = in.readString();
    mAboutExpert = in.readString();
    mOfferDescription = in.readString();
    mPhotoUrl = in.readString();
    mPageUrl = in.readString();
  }

  @Override
  public void writeToParcel(Parcel dest, int flags)
  {
    dest.writeInt(mId);
    dest.writeString(mName);
    dest.writeString(mCountry);
    dest.writeString(mCity);
    dest.writeDouble(mRating);
    dest.writeInt(mReviewCount);
    dest.writeDouble(mPrice);
    dest.writeString(mCurrency);
    dest.writeString(mMotto);
    dest.writeString(mAboutExpert);
    dest.writeString(mOfferDescription);
    dest.writeString(mPhotoUrl);
    dest.writeString(mPageUrl);
  }

  @Override
  public int describeContents() { return 0; }

  int getId() { return mId; }

  @NonNull
  String getName() { return mName; }

  @NonNull
  String getCountry() { return mCountry; }

  @NonNull
  String getCity() { return mCity; }

  double getRating() { return mRating; }

  int getReviewCount() { return mReviewCount; }

  double getPrice() { return mPrice; }

  @NonNull
  String getCurrency() { return mCurrency; }

  @NonNull
  String getMotto() { return mMotto; }

  @NonNull
  String getAboutExpert() { return mAboutExpert; }

  @NonNull
  String getOfferDescription() { return mOfferDescription; }

  @NonNull
  String getPageUrl() { return mPageUrl;}

  @NonNull
  String getPhotoUrl() { return mPhotoUrl; }

  @Override
  public boolean equals(Object o)
  {
    if (this == o) return true;
    if (o == null || getClass() != o.getClass()) return false;

    LocalExpert that = (LocalExpert) o;

    if (mId != that.mId) return false;
    if (Double.compare(that.mRating, mRating) != 0) return false;
    if (mReviewCount != that.mReviewCount) return false;
    if (Double.compare(that.mPrice, mPrice) != 0) return false;
    if (!mName.equals(that.mName)) return false;
    if (!mCountry.equals(that.mCountry)) return false;
    if (!mCity.equals(that.mCity)) return false;
    if (!mCurrency.equals(that.mCurrency)) return false;
    if (!mMotto.equals(that.mMotto)) return false;
    if (!mAboutExpert.equals(that.mAboutExpert)) return false;
    if (!mOfferDescription.equals(that.mOfferDescription)) return false;
    if (!mPageUrl.equals(that.mPageUrl)) return false;
    return mPhotoUrl.equals(that.mPhotoUrl);
  }

  @Override
  public int hashCode()
  {
    int result;
    long temp;
    result = mId;
    result = 31 * result + mName.hashCode();
    result = 31 * result + mCountry.hashCode();
    result = 31 * result + mCity.hashCode();
    temp = Double.doubleToLongBits(mRating);
    result = 31 * result + (int) (temp ^ (temp >>> 32));
    result = 31 * result + mReviewCount;
    temp = Double.doubleToLongBits(mPrice);
    result = 31 * result + (int) (temp ^ (temp >>> 32));
    result = 31 * result + mCurrency.hashCode();
    result = 31 * result + mMotto.hashCode();
    result = 31 * result + mAboutExpert.hashCode();
    result = 31 * result + mOfferDescription.hashCode();
    result = 31 * result + mPageUrl.hashCode();
    result = 31 * result + mPhotoUrl.hashCode();
    return result;
  }
}
