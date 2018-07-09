package com.mapswithme.maps.taxi;

import android.os.Parcel;
import android.os.Parcelable;
import android.support.annotation.NonNull;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class TaxiInfo implements Parcelable
{
  public static final Parcelable.Creator<TaxiInfo> CREATOR = new Parcelable.Creator<TaxiInfo>()
  {
    @Override
    public TaxiInfo createFromParcel(Parcel source)
    {
      return new TaxiInfo(source);
    }

    @Override
    public TaxiInfo[] newArray(int size)
    {
      return new TaxiInfo[size];
    }
  };

  @NonNull
  private final TaxiType mType;
  @NonNull
  private final List<Product> mProducts;

  private TaxiInfo(int type, @NonNull Product[] products)
  {
    mType = TaxiType.values()[type];
    mProducts = new ArrayList<>(Arrays.asList(products));
  }

  private TaxiInfo(@NonNull Parcel parcel)
  {
    //noinspection WrongConstant
    mType = TaxiType.values()[parcel.readInt()];
    List<Product> products = new ArrayList<>();
    parcel.readTypedList(products, Product.CREATOR);
    mProducts = products;
  }

  @NonNull
  public final TaxiType getType()
  {
    return mType;
  }

  @NonNull
  public List<Product> getProducts()
  {
    return mProducts;
  }

  @Override
  public String toString()
  {
    return "TaxiInfo{" +
           "mType=" + mType +
           ", mProducts=" + mProducts +
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
    dest.writeInt(mType.ordinal());
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
    @NonNull
    private final String mCurrency;

    private Product(@NonNull String productId, @NonNull String name, @NonNull String time,
                    @NonNull String price, @NonNull String currency)
    {
      mProductId = productId;
      mName = name;
      mTime = time;
      mPrice = price;
      mCurrency = currency;
    }

    private Product(@NonNull Parcel parcel)
    {
      mProductId = parcel.readString();
      mName = parcel.readString();
      mTime = parcel.readString();
      mPrice = parcel.readString();
      mCurrency = parcel.readString();
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

    @NonNull
    public String getCurrency()
    {
      return mCurrency;
    }

    @Override
    public String toString()
    {
      return "Product{" +
             "mProductId='" + mProductId + '\'' +
             ", mName='" + mName + '\'' +
             ", mTime='" + mTime + '\'' +
             ", mPrice='" + mPrice + '\'' +
             ", mCurrency='" + mCurrency + '\'' +
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
      dest.writeString(mCurrency);
    }
  }
}

