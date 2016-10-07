package com.mapswithme.maps.api.uber;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import java.util.Arrays;

public class UberInfo
{
  @Nullable
  private final Product[] mProducts;

  public UberInfo(@Nullable Product[] products)
  {
    mProducts = products;
  }

  @Nullable
  public Product[] getProducts()
  {
    return mProducts;
  }

  @Override
  public String toString()
  {
    return "UberInfo{" +
           "mProducts=" + Arrays.toString(mProducts) +
           '}';
  }

  public static class Product
  {
    @NonNull
    private final String mProductId;
    @NonNull
    private final String mName;
    @NonNull
    private final String mTime;
    @NonNull
    private final String mPrice;

    public Product(@NonNull String productId, @NonNull String name, @NonNull String time, @NonNull String price)
    {
      mProductId = productId;
      mName = name;
      mTime = time;
      mPrice = price;
    }

    @NonNull
    public String getProductId()
    {
      return mProductId;
    }

    @NonNull
    public String getName()
    {
      return mName;
    }

    @NonNull
    public String getTime()
    {
      return mTime;
    }

    @NonNull
    public String getPrice()
    {
      return mPrice;
    }

    @Override
    public String toString()
    {
      return "Product{" +
             "mProductId='" + mProductId + '\'' +
             ", mName='" + mName + '\'' +
             ", mTime='" + mTime + '\'' +
             ", mPrice='" + mPrice + '\'' +
             '}';
    }
  }
}

