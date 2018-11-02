package com.mapswithme.maps.bookmarks.data;

import android.os.Parcel;
import android.os.Parcelable;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

public class PaymentData implements Parcelable
{
  @NonNull
  private final String mServerId;
  @NonNull
  private final String mProductId;
  @NonNull
  private final String mName;
  @Nullable
  private final String mImgUrl;
  @NonNull
  private final String mAuthorName;

  public PaymentData(@NonNull String serverId, @NonNull String productId, @NonNull String name,
                     @Nullable String imgUrl, @NonNull String authorName)
  {
    mServerId = serverId;
    mProductId = productId;
    mName = name;
    mImgUrl = imgUrl;
    mAuthorName = authorName;
  }

  private PaymentData(Parcel in)
  {
    mServerId = in.readString();
    mProductId = in.readString();
    mName = in.readString();
    mImgUrl = in.readString();
    mAuthorName = in.readString();
  }

  public static final Creator<PaymentData> CREATOR = new Creator<PaymentData>()
  {
    @Override
    public PaymentData createFromParcel(Parcel in)
    {
      return new PaymentData(in);
    }

    @Override
    public PaymentData[] newArray(int size)
    {
      return new PaymentData[size];
    }
  };

  @NonNull
  public String getServerId()
  {
    return mServerId;
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

  @Nullable
  public String getImgUrl()
  {
    return mImgUrl;
  }

  @NonNull
  public String getAuthorName()
  {
    return mAuthorName;
  }

  @Override
  public int describeContents()
  {
    return 0;
  }

  @Override
  public void writeToParcel(Parcel dest, int flags)
  {
    dest.writeString(mServerId);
    dest.writeString(mProductId);
    dest.writeString(mName);
    dest.writeString(mImgUrl);
    dest.writeString(mAuthorName);
  }
}
