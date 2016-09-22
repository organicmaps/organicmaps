package com.mapswithme.maps.review;

import android.os.Parcel;
import android.os.Parcelable;

public class Review implements Parcelable {

  private final String mReview;

  private final String mReviewPositive;

  private final String mReviewNegative;

  private final String mAuthor;

  private final String mAuthorAvatar;

  private final float mRating;

  private final long mDate;

  public Review(String review, String reviewPositive, String reviewNegative, String author,
          String authorAvatar,
          float rating, long date) {
    mReview = review;
    mReviewPositive = reviewPositive;
    mReviewNegative = reviewNegative;
    mAuthor = author;
    mAuthorAvatar = authorAvatar;
    mRating = rating;
    mDate = date;
  }

  protected Review(Parcel in) {
    mReview = in.readString();
    mReviewPositive = in.readString();
    mReviewNegative = in.readString();
    mAuthor = in.readString();
    mAuthorAvatar = in.readString();
    mRating = in.readFloat();
    mDate = in.readLong();
  }

  @Override
  public void writeToParcel(Parcel dest, int flags) {
    dest.writeString(mReview);
    dest.writeString(mReviewPositive);
    dest.writeString(mReviewNegative);
    dest.writeString(mAuthor);
    dest.writeString(mAuthorAvatar);
    dest.writeFloat(mRating);
    dest.writeLong(mDate);
  }

  @Override
  public int describeContents() {
    return 0;
  }

  public static final Creator<Review> CREATOR = new Creator<Review>() {
    @Override
    public Review createFromParcel(Parcel in) {
      return new Review(in);
    }

    @Override
    public Review[] newArray(int size) {
      return new Review[size];
    }
  };

  public String getReview() {
    return mReview;
  }

  public String getReviewPositive() {
    return mReviewPositive;
  }

  public String getReviewNegative() {
    return mReviewNegative;
  }

  public String getAuthor() {
    return mAuthor;
  }

  @SuppressWarnings("unused")
  public String getAuthorAvatar() {
    return mAuthorAvatar;
  }

  public float getRating() {
    return mRating;
  }

  public long getDate() {
    return mDate;
  }
}
