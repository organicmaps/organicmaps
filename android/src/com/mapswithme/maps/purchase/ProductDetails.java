package com.mapswithme.maps.purchase;

import android.os.Parcel;
import android.os.Parcelable;
import android.support.annotation.NonNull;

class ProductDetails implements Parcelable
{
  @NonNull
  private final String mProductId;
  private final float mPrice;
  @NonNull
  private final String mCurrencyCode;
  @NonNull
  private final String mTitle;

  ProductDetails(@NonNull String productId, float price, @NonNull String currencyCode, @NonNull
      String title)
  {
    mProductId = productId;
    mPrice = price;
    mCurrencyCode = currencyCode;
    mTitle = title;
  }

  private ProductDetails(Parcel in)
  {
    this(in.readString(), in.readFloat(), in.readString(), in.readString());
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
  }
}
