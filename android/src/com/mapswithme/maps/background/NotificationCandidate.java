package com.mapswithme.maps.background;

import androidx.annotation.IntDef;
import androidx.annotation.NonNull;

import java.io.Serializable;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

public class NotificationCandidate implements Serializable
{
  private static final long serialVersionUID = -7020549752940235436L;

  // This constants should be compatible with notifications::NotificationCandidate::Type enum
  // from c++ side.
  static final int TYPE_UGC_AUTH = 0;
  static final int TYPE_UGC_REVIEW = 1;

  @Retention(RetentionPolicy.SOURCE)
  @IntDef({ TYPE_UGC_AUTH, TYPE_UGC_REVIEW })
  @interface NotificationType
  {
  }

  public static class UgcReview extends NotificationCandidate
  {
    private static final long serialVersionUID = 5469867251355445859L;

    private final double mMercatorPosX;
    private final double mMercatorPosY;
    @NonNull
    private final String mReadableName;
    @NonNull
    private final String mDefaultName;
    @NonNull
    private final String mFeatureBestType;
    @NonNull
    private final String mAddress;

    @SuppressWarnings("unused")
    UgcReview(double posX, double posY, @NonNull String readableName, @NonNull String defaultName,
              @NonNull String bestType, @NonNull String address)
    {
      super(TYPE_UGC_REVIEW);

      mMercatorPosX = posX;
      mMercatorPosY = posY;
      mReadableName = readableName;
      mDefaultName = defaultName;
      mFeatureBestType = bestType;
      mAddress = address;
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
    public String getFeatureBestType()
    {
      return mFeatureBestType;
    }

    @NonNull
    public String getAddress()
    {
      return mAddress;
    }
  }

  @NotificationType
  private final int mType;

  private NotificationCandidate(@NotificationType int type)
  {
    mType = type;
  }

  public int getType()
  {
    return mType;
  }
}
