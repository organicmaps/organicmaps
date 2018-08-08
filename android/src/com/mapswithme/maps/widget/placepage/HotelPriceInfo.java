package com.mapswithme.maps.widget.placepage;

import android.support.annotation.NonNull;

public class HotelPriceInfo
{
  @NonNull
  private final String mId;
  @NonNull
  private final String mPrice;
  @NonNull
  private final String mCurrency;
  private final int mDiscount;
  private final boolean mHasSmartDeal;

  public HotelPriceInfo(@NonNull String id, @NonNull String price, @NonNull String currency,
                        int discount, boolean hasSmartDeal)
  {
    mId = id;
    mPrice = price;
    mCurrency = currency;
    mDiscount = discount;
    mHasSmartDeal = hasSmartDeal;
  }

  @NonNull
  public String getId()
  {
    return mId;
  }

  @NonNull
  public String getPrice()
  {
    return mPrice;
  }

  @NonNull
  public String getCurrency()
  {
    return mCurrency;
  }

  public int getDiscount()
  {
    return mDiscount;
  }

  public boolean hasSmartDeal()
  {
    return mHasSmartDeal;
  }

  @Override
  public String toString()
  {
    final StringBuilder sb = new StringBuilder("HotelPriceInfo{");
    sb.append("mId='").append(mId).append('\'');
    sb.append(", mPrice='").append(mPrice).append('\'');
    sb.append(", mCurrency='").append(mCurrency).append('\'');
    sb.append(", mDiscount=").append(mDiscount);
    sb.append(", mHasSmartDeal=").append(mHasSmartDeal);
    sb.append('}');
    return sb.toString();
  }
}
