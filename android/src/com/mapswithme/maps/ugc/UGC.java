package com.mapswithme.maps.ugc;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import com.mapswithme.util.concurrency.UiThread;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

public class UGC
{

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
  List<Rating> getRatings()
  {
    return Collections.synchronizedList(Arrays.asList(mRatings));
  }

  @Nullable
  List<Review> getReviews()
  {
    if (mReviews == null)
      return null;

    return Collections.synchronizedList(Arrays.asList(mReviews));
  }

  //TODO: remove static
  public static void requestUGC(@NonNull String featureId)
  {
    //TODO: mock implementation
    final List<Review> reviews = new ArrayList<>();
    reviews.add(new Review("Great cafe! Fish is the best:)", "Derick Naef", System.currentTimeMillis()));
    reviews.add(new Review("Good! Very good store! Never been here before!!!!!!!!!!!!!!!!!! :((( Fish is the best:)",
                           "Katie Colins", System.currentTimeMillis()));
    reviews.add(new Review("Horrible service that I've ever obtained in Russia! Smell and waitress are crazy!",
                           "Jam Fox", System.currentTimeMillis()));
    if (mListener != null)
      UiThread.runLater(new Runnable()
      {
        @Override
        public void run()
        {
          mListener.onUGCReviewsObtained(reviews);
        }
      }, 500);

  }

  public static void setListener(@Nullable UGCListener listener)
  {
    mListener = listener;
  }

  public static class Rating
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

  public static class Review
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
    void onUGCReviewsObtained(@NonNull List<Review> reviews);
    void onUGCRatingsObtained(@NonNull List<Rating> ratings);
  }
}
