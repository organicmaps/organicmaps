package com.mapswithme.maps.ugc;

import android.support.annotation.IntDef;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import java.io.Serializable;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
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

  public static native void requestUGC();

  public static void onUGCReceived(@NonNull UGC ugc)
  {
    if (mListener != null)
      mListener.onUGCReceived(ugc);
  }

  public static class Rating implements Serializable
  {
    @NonNull
    private final String mName;
    private final float mValue;

    private Rating(@NonNull String name, float value)
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
  }

  public static class Review implements Serializable
  {
    @NonNull
    private final String mText;
    @NonNull
    private final String mAuthor;
    private final long mTime;

    private Review(@NonNull String text, @NonNull String author, long time)
    {
      mText = text;
      mAuthor = author;
      mTime = time;
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

    public long getTime()
    {
      return mTime;
    }
  }

  public interface UGCListener
  {
    void onUGCReceived(@NonNull UGC ugc);
  }
}
