package com.mapswithme.maps.purchase;

import android.os.Parcel;
import android.os.Parcelable;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

class ProductDetails implements Parcelable
{
  @NonNull
  private final String mProductId;
  private final float mPrice;
  @NonNull
  private final String mCurrencyCode;
  @NonNull
  private final String mTitle;
  @Nullable
  private final String mFreeTrialPeriod;

  ProductDetails(@NonNull String productId, float price, @NonNull String currencyCode,
                 @NonNull String title, @Nullable String freeTrialPeriod)
  {
    mProductId = productId;
    mPrice = price;
    mCurrencyCode = currencyCode;
    mTitle = title;
    mFreeTrialPeriod = freeTrialPeriod;
  }

  private ProductDetails(Parcel in)
  {
    this(in.readString(), in.readFloat(), in.readString(), in.readString(), in.readString());
  }

  float getPrice()
  {
    return mPrice;
  }

  @NonNull
  String getCurrencyCode()
  {
    return mCurrencyCode;
  }

  @NonNull
  String getProductId()
  {
    return mProductId;
  }

  @NonNull
  public String getTitle()
  {
    return mTitle;
  }

  @Nullable
  String getFreeTrialPeriod()
  {
    return mFreeTrialPeriod;
  }

  public static final Creator<ProductDetails> CREATOR = new Creator<ProductDetails>()
  {
    @Override
    public ProductDetails createFromParcel(Parcel in)
    {
      return new ProductDetails(in);
    }

    @Override
    public ProductDetails[] newArray(int size)
    {
      return new ProductDetails[size];
    }
  };

  @Override
  public int describeContents()
  {
    return 0;
  }

  @Override
  public void writeToParcel(Parcel dest, int flags)
  {
    dest.writeString(mProductId);
    dest.writeFloat(mPrice);
    dest.writeString(mCurrencyCode);
    dest.writeString(mTitle);
    dest.writeString(mFreeTrialPeriod);
  }
}
