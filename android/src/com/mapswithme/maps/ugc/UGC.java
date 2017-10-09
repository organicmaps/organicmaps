package com.mapswithme.maps.ugc;

import android.support.annotation.IntDef;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import com.mapswithme.maps.bookmarks.data.FeatureId;

import java.io.Serializable;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

//TODO: make it Parcelable instead of Serializable
public class UGC implements Serializable
{
  @Retention(RetentionPolicy.SOURCE)
  @IntDef({ RATING_NONE, RATING_HORRIBLE, RATING_BAD, RATING_NORMAL, RATING_GOOD,
            RATING_EXCELLENT, RATING_COMING_SOON })

  public @interface Impress
  {}

  static final int RATING_NONE = 0;
  static final int RATING_HORRIBLE = 1;
  static final int RATING_BAD = 2;
  static final int RATING_NORMAL = 3;
  static final int RATING_GOOD = 4;
  static final int RATING_EXCELLENT = 5;
  static final int RATING_COMING_SOON = 6;

  @NonNull
  private final Rating[] mRatings;
  @Nullable
  private final Review[] mReviews;
  private final int mBasedOnCount;
  private final float mAverageRating;
  @Nullable
  private static UGCListener mListener;

  private UGC(@NonNull Rating[] ratings, float averageRating, @Nullable Review[] reviews,
              int basedOnCount)
  {
    mRatings = ratings;
    mReviews = reviews;
    mAverageRating = averageRating;
    mBasedOnCount = basedOnCount;
  }

  int getBasedOnCount()
  {
    return mBasedOnCount;
  }

  @NonNull
  List<Rating> getRatings()
  {
    return Collections.synchronizedList(Arrays.asList(mRatings));
  }

  //TODO: remove it after core is ready.
  @NonNull
  List<Rating> getUserRatings()
  {
    return new ArrayList<Rating>(){
      {
        add(new Rating("service", 3.3f));
        add(new Rating("food", 4.4f));
        add(new Rating("quality", 2.0f));
        add(new Rating("cleaning", 3.9f));
      }
    };
  }

  @Nullable
  public List<Review> getReviews()
  {
    if (mReviews == null)
      return null;

    return Collections.synchronizedList(Arrays.asList(mReviews));
  }

  public static void setListener(@Nullable UGCListener listener)
  {
    mListener = listener;
  }

  public static native void requestUGC(@NonNull FeatureId fid);

  public static native void setUGCUpdate(@NonNull FeatureId fid, UGCUpdate update);

  public static void onUGCReceived(@Nullable UGC ugc, @Nullable UGCUpdate ugcUpdate,
                                   @Impress int impress, @NonNull String rating)
  {
    if (mListener != null)
    {
      if (ugc == null && ugcUpdate != null)
        impress = UGC.RATING_COMING_SOON;
      mListener.onUGCReceived(ugc, ugcUpdate, impress, rating);
    }
  }

  public static class Rating implements Serializable
  {
    @NonNull
    private final String mName;
    private float mValue;

    Rating(@NonNull String name, float value)
    {
      mName = name;
      mValue = value;
    }

    public float getValue()
    {
      return mValue;
    }

    @NonNull
    public String getName()
    {
      return mName;
    }

    public void setValue(float value)
    {
      mValue = value;
    }
  }

  public static class Review implements Serializable
  {
    @NonNull
    private final String mText;
    @NonNull
    private final String mAuthor;
    private final long mDaysAgo;

    private Review(@NonNull String text, @NonNull String author, long daysAgo)
    {
      mText = text;
      mAuthor = author;
      mDaysAgo = daysAgo;
    }

    @NonNull
    public String getText()
    {
      return mText;
    }

    @NonNull
    String getAuthor()
    {
      return mAuthor;
    }

    long getDaysAgo()
    {
      return mDaysAgo;
    }
  }

  interface UGCListener
  {
    void onUGCReceived(@Nullable UGC ugc, @Nullable UGCUpdate ugcUpdate, @Impress int impress,
                       @NonNull String rating);
  }
}
