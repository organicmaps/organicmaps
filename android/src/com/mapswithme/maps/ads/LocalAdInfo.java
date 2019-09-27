package com.mapswithme.maps.ads;

import android.os.Parcel;
import android.os.Parcelable;
import androidx.annotation.IntDef;
import androidx.annotation.Nullable;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

public class LocalAdInfo implements Parcelable
{
  @Retention(RetentionPolicy.SOURCE)
  @IntDef({ STATUS_NOT_AVAILABLE, STATUS_CANDIDATE, STATUS_CUSTOMER, STATUS_HIDDEN})

  public @interface Status {}

  private static final int STATUS_NOT_AVAILABLE = 0;
  private static final int STATUS_CANDIDATE = 1;
  private static final int STATUS_CUSTOMER = 2;
  private static final int STATUS_HIDDEN = 3;

  @Status
  private final int mStatus;
  @Nullable
  private final String mUrl;

  private LocalAdInfo(@Status int status, @Nullable String url)
  {
    mUrl = url;
    mStatus = status;
  }

  public boolean isAvailable()
  {
    return mStatus != STATUS_NOT_AVAILABLE;
  }

  public boolean isCustomer()
  {
    return mStatus == STATUS_CUSTOMER;
  }

  public boolean isHidden()
  {
    return mStatus == STATUS_HIDDEN;
  }

  @Nullable
  public String getUrl()
  {
    return mUrl;
  }

  private LocalAdInfo(Parcel in)
  {
    //noinspection WrongConstant
    this(in.readInt() /* mStatus */, in.readString() /* mUrl */);
  }

  public static final Creator<LocalAdInfo> CREATOR = new Creator<LocalAdInfo>()
  {
    @Override
    public LocalAdInfo createFromParcel(Parcel in)
    {
      return new LocalAdInfo(in);
    }

    @Override
    public LocalAdInfo[] newArray(int size)
    {
      return new LocalAdInfo[size];
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
    dest.writeInt(mStatus);
    dest.writeString(mUrl);
  }
}
