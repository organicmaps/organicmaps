package com.mapswithme.maps.uber;

import android.os.Parcel;
import android.os.Parcelable;
import android.support.annotation.NonNull;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class UberInfo implements Parcelable
{
  public static final Parcelable.Creator<UberInfo> CREATOR = new Parcelable.Creator<UberInfo>()
  {
    @Override
    public UberInfo createFromParcel(Parcel source)
    {
      return new UberInfo(source);
    }

    @Override
    public UberInfo[] newArray(int size)
    {
      return new UberInfo[size];
    }
  };

  @NonNull
  private final List<Product> mProducts;

  private UberInfo(@NonNull Product[] products)
  {
    mProducts = new ArrayList<>(Arrays.asList(products));
  }

  private UberInfo(@NonNull Parcel parcel)
  {
    List<Product> products = new ArrayList<>();
    parcel.readTypedList(products, Product.CREATOR);
    mProducts = products;
  }

  @NonNull
  public List<Product> getProducts()
  {
    return mProducts;
  }

  @Override
  public String toString()
  {
    return "UberInfo{" +
           "mProducts=" + mProducts +
           '}';
  }

  @Override
  public int describeContents()
  {
    return 0;
  }

  @Override
  public void writeToParcel(Parcel dest, int flags)
  {
    dest.writeTypedList(mProducts);
  }

  public static class Product implements Parcelable
  {
    public static final Parcelable.Creator<Product> CREATOR = new Parcelable.Creator<Product>()
    {
      @Override
      public Product createFromParcel(Parcel source)
      {
        return new Product(source);
      }

      @Override
      public Product[] newArray(int size)
      {
        return new Product[size];
      }
    };

    @NonNull
    private final String mProductId;
    @NonNull
    private final String mName;
    @NonNull
    private final String mTime;
    @NonNull
    private final String mPrice;

    private Product(@NonNull String productId, @NonNull String name, @NonNull String time, @NonNull String price)
    {
      mProductId = productId;
      mName = name;
      mTime = time;
      mPrice = price;
    }

    private Product(@NonNull Parcel parcel)
    {
      mProductId = parcel.readString();
      mName = parcel.readString();
      mTime = parcel.readString();
      mPrice = parcel.readString();
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

    @Override
    public int describeContents()
    {
      return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags)
    {
      dest.writeString(mProductId);
      dest.writeString(mName);
      dest.writeString(mTime);
      dest.writeString(mPrice);
    }
  }
}

