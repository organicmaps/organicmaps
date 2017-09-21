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
  @IntDef({ RATING_HORRIBLE, RATING_BAD, RATING_NORMAL, RATING_GOOD, RATING_EXCELLENT })

  public @interface UGCRating
  {}

  public static final int RATING_HORRIBLE = 1;
  public static final int RATING_BAD = 2;
  public static final int RATING_NORMAL = 3;
  public static final int RATING_GOOD = 4;
  public static final int RATING_EXCELLENT = 5;

  @NonNull
  private final Rating[] mRatings;
  @Nullable
  private final Review[] mReviews;
  private final float mAverageRating;
  @Nullable
  private static UGCListener mListener;

  private UGC(@NonNull Rating[] ratings, float averageRating, @Nullable Review[] reviews)
  {
    mRatings = ratings;
    mReviews = reviews;
    mAverageRating = averageRating;
  }

  @NonNull
  public List<Rating> getRatings()
  {
    return Collections.synchronizedList(Arrays.asList(mRatings));
  }

  //TODO: remove it after core is ready.
  @NonNull
  public List<Rating> getUserRatings()
  {
    return new ArrayList<Rating>(){
      {
        add(new Rating("service", 8.3f));
        add(new Rating("food", 3.4f));
        add(new Rating("quality", 5.0f));
        add(new Rating("cleaning", 7.9f));
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

  public static native void requestUGC(FeatureId fid);

  public static void onUGCReceived(@NonNull UGC ugc)
  {
    if (mListener != null)
      mListener.onUGCReceived(ugc);
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
    public String getAuthor()
    {
      return mAuthor;
    }

    public long getDaysAgo()
    {
      return mDaysAgo;
    }
  }

  public interface UGCListener
  {
    void onUGCReceived(@NonNull UGC ugc);
  }
}
