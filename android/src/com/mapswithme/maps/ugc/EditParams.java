package com.mapswithme.maps.ugc;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import com.mapswithme.maps.bookmarks.data.FeatureId;

import java.util.ArrayList;

class EditParams
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
    private double mLat;
    private double mLon;
    @Nullable
    private String mAddress;

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

    Builder setDefaultRating(@UGC.Impress int defaultRating)
    {
      mDefaultRating = defaultRating;
      return this;
    }

    Builder setCanBeReviewed(boolean value)
    {
      mCanBeReviewed = value;
      return this;
    }

    Builder setFromPP(boolean value)
    {
      mFromPP = value;
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
