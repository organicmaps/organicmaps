package com.mapswithme.maps.ugc;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import com.mapswithme.maps.bookmarks.data.FeatureId;
import com.mapswithme.maps.bookmarks.data.MapObject;

import java.util.ArrayList;

public class EditParams
{
  @NonNull
  private final String mTitle;
  @NonNull
  private final FeatureId mFeatureId;
  @Nullable
  private final ArrayList<UGC.Rating> mRatings;
  @UGC.Impress
  private final int mDefaultRating;
  private final boolean mCanBeReviewed;
  private final boolean mFromPP;
  private final boolean mFromNotification;
  // TODO: mLat, mLon, mAddress are added just for debugging null feature id for ugc object.
  // Remove they after problem is fixed.
  private double mLat;
  private double mLon;
  @Nullable
  private String mAddress;

  private EditParams(@NonNull Builder builder)
  {
    mTitle = builder.mTitle;
    mFeatureId = builder.mFeatureId;
    mRatings = builder.mRatings;
    mDefaultRating = builder.mDefaultRating;
    mCanBeReviewed = builder.mCanBeReviewed;
    mFromPP = builder.mFromPP;
    mFromNotification = builder.mFromNotification;
    mLat = builder.mLat;
    mLon = builder.mLon;
    mAddress = builder.mAddress;
  }

  @NonNull
  public String getTitle()
  {
    return mTitle;
  }

  @NonNull
  public FeatureId getFeatureId()
  {
    return mFeatureId;
  }

  @Nullable
  public ArrayList<UGC.Rating> getRatings()
  {
    return mRatings;
  }

  int getDefaultRating()
  {
    return mDefaultRating;
  }

  boolean canBeReviewed()
  {
    return mCanBeReviewed;
  }

  boolean isFromPP()
  {
    return mFromPP;
  }

  boolean isFromNotification()
  {
    return mFromNotification;
  }

  double getLat()
  {
    return mLat;
  }

  double getLon()
  {
    return mLon;
  }

  @Nullable
  String getAddress()
  {
    return mAddress;
  }

  public static class Builder
  {
    @NonNull
    private final String mTitle;
    @NonNull
    private final FeatureId mFeatureId;
    @Nullable
    private ArrayList<UGC.Rating> mRatings;
    @UGC.Impress
    private int mDefaultRating;
    private boolean mCanBeReviewed;
    private boolean mFromPP;
    private boolean mFromNotification;
    private double mLat;
    private double mLon;
    @Nullable
    private String mAddress;

    @NonNull
    public static EditParams.Builder fromMapObject(@NonNull MapObject mapObject)
    {
      return new EditParams.Builder(mapObject.getTitle(), mapObject.getFeatureId())
        .setRatings(mapObject.getDefaultRatings())
        .setCanBeReviewed(mapObject.canBeReviewed())
        .setLat(mapObject.getLat())
        .setLon(mapObject.getLon())
        .setAddress(mapObject.getAddress());
    }

    public Builder(@NonNull String title, @NonNull FeatureId featureId)
    {
      mTitle = title;
      mFeatureId = featureId;
    }

    public Builder setRatings(@Nullable ArrayList<UGC.Rating> ratings)
    {
      mRatings = ratings;
      return this;
    }

    public Builder setDefaultRating(@UGC.Impress int defaultRating)
    {
      mDefaultRating = defaultRating;
      return this;
    }

    public Builder setCanBeReviewed(boolean value)
    {
      mCanBeReviewed = value;
      return this;
    }

    public Builder setFromPP(boolean value)
    {
      mFromPP = value;
      return this;
    }

    public Builder setFromNotification(boolean value)
    {
      mFromNotification = value;
      return this;
    }

    public Builder setLat(double lat)
    {
      mLat = lat;
      return this;
    }

    public Builder setLon(double lon)
    {
      mLon = lon;
      return this;
    }

    public Builder setAddress(@Nullable String address)
    {
      mAddress = address;
      return this;
    }

    public EditParams build()
    {
      return new EditParams(this);
    }
  }
}
