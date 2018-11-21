package com.mapswithme.maps.ugc;

import android.content.Context;
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
import java.util.Arrays;
import java.util.List;

public class UGC
{
  @Retention(RetentionPolicy.SOURCE)
  @IntDef({ RATING_NONE, RATING_HORRIBLE, RATING_BAD, RATING_NORMAL, RATING_GOOD,
            RATING_EXCELLENT, RATING_COMING_SOON })

  public @interface Impress
  {}

  public static final int RATING_NONE = 0;
  public static final int RATING_HORRIBLE = 1;
  public static final int RATING_BAD = 2;
  public static final int RATING_NORMAL = 3;
  public static final int RATING_GOOD = 4;
  public static final int RATING_EXCELLENT = 5;
  public static final int RATING_COMING_SOON = 6;

  @NonNull
  private final Rating[] mRatings;
  @Nullable
  private final Review[] mReviews;
  private final int mBasedOnCount;
  private final float mAverageRating;
  @Nullable
  private static UGCListener mListener;

  public static void init(final @NonNull Context context)
  {
    final AppBackgroundTracker.OnTransitionListener listener = new UploadUgcTransitionListener(context);
    MwmApplication.backgroundTracker().addListener(listener);
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
    return Arrays.asList(mRatings);
  }

  @Nullable
  public List<Review> getReviews()
  {
    if (mReviews == null)
      return null;

    return Arrays.asList(mReviews);
  }

  public static void setListener(@Nullable UGCListener listener)
  {
    mListener = listener;
  }

  public static native void requestUGC(@NonNull FeatureId fid);

  public static native void setUGCUpdate(@NonNull FeatureId fid, UGCUpdate update);

  public static native void nativeUploadUGC();

  public static native int nativeToImpress(float rating);

  @NonNull
  public static native String nativeFormatRating(float rating);

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

  private static class UploadUgcTransitionListener implements AppBackgroundTracker.OnTransitionListener
  {
    @NonNull
    private final Context mContext;

    UploadUgcTransitionListener(@NonNull Context context)
    {
      mContext = context;
    }

    @Override
    public void onTransit(boolean foreground)
    {
      if (foreground)
        return;

      WorkerService.startActionUploadUGC(mContext);
    }
  }
}
