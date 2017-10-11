package com.mapswithme.maps.ugc;

import android.os.Parcel;
import android.os.Parcelable;
import android.support.annotation.IntDef;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.background.AppBackgroundTracker;
import com.mapswithme.maps.background.WorkerService;
import com.mapswithme.maps.bookmarks.data.FeatureId;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

public class UGC
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

  private static final AppBackgroundTracker.OnTransitionListener UPLOADER =
      new AppBackgroundTracker.OnTransitionListener()
  {
    @Override
    public void onTransit(boolean foreground)
    {
      if (!foreground)
        WorkerService.startActionUploadUGC();
    }
  };

  @NonNull
  private final Rating[] mRatings;
  @Nullable
  private final Review[] mReviews;
  private final int mBasedOnCount;
  private final float mAverageRating;
  @Nullable
  private static UGCListener mListener;

  public static void init()
  {
    MwmApplication.backgroundTracker().addListener(UPLOADER);
  }

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
  static ArrayList<Rating> getUserRatings()
  {
    return new ArrayList<Rating>(){
      {
        add(new Rating("cuisine", 3.3f));
        add(new Rating("service", 4.4f));
        add(new Rating("atmosphere", 2.0f));
        add(new Rating("experience", 3.9f));
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

  public static native void nativeUploadUGC();

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

  public static class Rating implements Parcelable
  {
    public static final Creator<Rating> CREATOR = new Creator<Rating>()
    {
      @Override
      public Rating createFromParcel(Parcel in)
      {
        return new Rating(in);
      }

      @Override
      public Rating[] newArray(int size)
      {
        return new Rating[size];
      }
    };

    @NonNull
    private final String mName;
    private float mValue;

    Rating(@NonNull String name, float value)
    {
      mName = name;
      mValue = value;
    }

    protected Rating(Parcel in)
    {
      mName = in.readString();
      mValue = in.readFloat();
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

    @Override
    public int describeContents()
    {
      return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags)
    {
      dest.writeString(mName);
      dest.writeFloat(mValue);
    }
  }

  public static class Review implements Parcelable
  {
    public static final Creator<Review> CREATOR = new Creator<Review>()
    {
      @Override
      public Review createFromParcel(Parcel in)
      {
        return new Review(in);
      }

      @Override
      public Review[] newArray(int size)
      {
        return new Review[size];
      }
    };

    @NonNull
    private final String mText;
    @NonNull
    private final String mAuthor;
    private final long mTimeMillis;
    private final float mRating;
    @Impress
    private final int mImpress;

    private Review(@NonNull String text, @NonNull String author, long timeMillis,
                   float rating, @Impress int impress)
    {
      mText = text;
      mAuthor = author;
      mTimeMillis = timeMillis;
      mRating = rating;
      mImpress = impress;
    }

    protected Review(Parcel in)
    {
      mText = in.readString();
      mAuthor = in.readString();
      mTimeMillis = in.readLong();
      mRating = in.readFloat();
      //noinspection WrongConstant
      mImpress = in.readInt();
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

    long getTime()
    {
      return mTimeMillis;
    }

    public float getRating()
    {
      return mRating;
    }

    int getImpress()
    {
      return mImpress;
    }

    @Override
    public int describeContents()
    {
      return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags)
    {
      dest.writeString(mText);
      dest.writeString(mAuthor);
      dest.writeLong(mTimeMillis);
      dest.writeFloat(mRating);
      dest.writeInt(mImpress);
    }
  }

  interface UGCListener
  {
    void onUGCReceived(@Nullable UGC ugc, @Nullable UGCUpdate ugcUpdate, @Impress int impress,
                       @NonNull String rating);
  }
}
