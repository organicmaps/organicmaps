package com.mapswithme.maps.widget.placepage;

import androidx.annotation.NonNull;

public class HotelPriceInfo
{
  @NonNull
  private final String mId;
  @NonNull
  private final String mPrice;
  @NonNull
  private final String mCurrency;
  private final int mDiscount;
  private final boolean mSmartDeal;

  public HotelPriceInfo(@NonNull String id, @NonNull String price, @NonNull String currency,
                        int discount, boolean smartDeal)
  {
    mId = id;
    mPrice = price;
    mCurrency = currency;
    mDiscount = discount;
    mSmartDeal = smartDeal;
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
    return mSmartDeal;
  }

  @Override
  public String toString()
  {
    final StringBuilder sb = new StringBuilder("HotelPriceInfo{");
    sb.append("mId='").append(mId).append('\'');
    sb.append(", mPrice='").append(mPrice).append('\'');
    sb.append(", mCurrency='").append(mCurrency).append('\'');
    sb.append(", mDiscount=").append(mDiscount);
    sb.append(", mSmartDeal=").append(mSmartDeal);
    sb.append('}');
    return sb.toString();
  }
}
