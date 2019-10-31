package com.mapswithme.maps.purchase;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

public class SubsButtonEntity
{
  @NonNull
  private final String mName;

  @NonNull
  private final String mPrice;

  @Nullable
  private final String mSale;

  SubsButtonEntity(@NonNull String name, @NonNull String price, @Nullable String sale)
  {
    mName = name;
    mPrice = price;
    mSale = sale;
  }

  SubsButtonEntity(@NonNull String name, @NonNull String price)
  {
    this(name, price, null);
  }

  @NonNull
  public String getName()
  {
    return mName;
  }

  @NonNull
  public String getPrice()
  {
    return mPrice;
  }

  @Nullable
  public String getSale()
  {
    return mSale;
  }
}
