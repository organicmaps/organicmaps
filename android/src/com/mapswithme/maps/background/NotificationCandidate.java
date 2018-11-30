package com.mapswithme.maps.background;

import android.os.Parcel;
import android.os.Parcelable;
import android.support.annotation.IntDef;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import java.io.Serializable;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

public class NotificationCandidate
{
  // This constants should be compatible with notifications::NotificationCandidate::Type enum
  // from c++ side.
  static final int TYPE_UGC_AUTH = 0;
  static final int TYPE_UGC_REVIEW = 1;

  @Retention(RetentionPolicy.SOURCE)
  @IntDef({ TYPE_UGC_AUTH, TYPE_UGC_REVIEW })
  @interface NotificationType {}

  public static class MapObject implements Parcelable, Serializable
  {
    private static final long serialVersionUID = -7443680760782198916L;
    private final double mMercatorPosX;
    private final double mMercatorPosY;
    @NonNull
    private final String mReadableName;
    @NonNull
    private final String mDefaultName;
    @NonNull
    private final String mBestType;

    public static final Creator<MapObject> CREATOR = new Creator<MapObject>()
    {
      @Override
      public MapObject createFromParcel(Parcel in)
      {
        return new MapObject(in);
      }

      @Override
      public MapObject[] newArray(int size)
      {
        return new MapObject[size];
      }
    };

    @SuppressWarnings("unused")
    MapObject(double posX, double posY, @NonNull String readableName, @NonNull String defaultName,
              @NonNull String bestType)
    {
      mMercatorPosX = posX;
      mMercatorPosY = posY;
      mReadableName = readableName;
      mDefaultName = defaultName;
      mBestType = bestType;
    }

    protected MapObject(Parcel in)
    {
      mMercatorPosX = in.readDouble();
      mMercatorPosY = in.readDouble();
      mReadableName = in.readString();
      mDefaultName = in.readString();
      mBestType = in.readString();
    }

    @Override
    public void writeToParcel(Parcel dest, int flags)
    {
      dest.writeDouble(mMercatorPosX);
      dest.writeDouble(mMercatorPosY);
      dest.writeString(mReadableName);
      dest.writeString(mDefaultName);
      dest.writeString(mBestType);
    }

    @Override
    public int describeContents()
    {
      return 0;
    }

    @SuppressWarnings("unused")
    public double getMercatorPosX()
    {
      return mMercatorPosX;
    }

    @SuppressWarnings("unused")
    public double getMercatorPosY()
    {
      return mMercatorPosY;
    }

    @NonNull
    public String getReadableName()
    {
      return mReadableName;
    }

    @NonNull
    public String getDefaultName()
    {
      return mDefaultName;
    }

    @NonNull
    @SuppressWarnings("unused")
    public String getBestType()
    {
      return mBestType;
    }
  }

  @NotificationType
  private final int mType;

  @Nullable
  private MapObject mMapObject;

  @SuppressWarnings("unused")
  NotificationCandidate(@NotificationType int type)
  {
    mType = type;
  }

  @SuppressWarnings("unused")
  NotificationCandidate(@NotificationType int type, @Nullable MapObject mapObject)
  {
    this(type);
    mMapObject = mapObject;
  }

  public int getType()
  {
    return mType;
  }

  @Nullable
  public MapObject getMapObject()
  {
    return mMapObject;
  }
}
