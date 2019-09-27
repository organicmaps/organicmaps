package com.mapswithme.maps.review;

import android.os.Parcel;
import android.os.Parcelable;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

public class Review implements Parcelable
{
  private final long mDate;
  private final float mRating;
  @NonNull
  private final String mAuthor;
  @Nullable
  private final String mPros;
  @Nullable
  private final String mCons;

  public Review(long date, float rating, @NonNull String author, @Nullable String pros,
                @Nullable String cons)
  {
    mDate = date;
    mRating = rating;
    mAuthor = author;
    mPros = pros;
    mCons = cons;
  }

  protected Review(Parcel in)
  {
    mDate = in.readLong();
    mRating = in.readFloat();
    mAuthor = in.readString();
    mPros = in.readString();
    mCons = in.readString();
  }

  @Override
  public void writeToParcel(Parcel dest, int flags)
  {
    dest.writeLong(mDate);
    dest.writeFloat(mRating);
    dest.writeString(mAuthor);
    dest.writeString(mPros);
    dest.writeString(mCons);
  }

  @Override
  public int describeContents()
  {
    return 0;
  }

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

  @Nullable
  public String getPros()
  {
    return mPros;
  }

  @Nullable
  public String getCons()
  {
    return mCons;
  }

  @NonNull
  public String getAuthor()
  {
    return mAuthor;
  }

  public float getRating()
  {
    return mRating;
  }

  public long getDate()
  {
    return mDate;
  }
}
