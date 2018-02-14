package com.mapswithme.maps.bookmarks.data;

public class Track
{
  private final long mTrackId;
  private final long mCategoryId;
  private final String mName;
  private final String mLengthString;
  private final int mColor;

  Track(long trackId, long categoryId, String name, String lengthString, int color)
  {
    mTrackId = trackId;
    mCategoryId = categoryId;
    mName = name;
    mLengthString = lengthString;
    mColor = color;
  }

  public String getName() { return mName; }

  public String getLengthString() { return mLengthString;}

  public int getColor() { return mColor; }

  public long getTrackId() { return mTrackId; }

  public long getCategoryId() { return mCategoryId; }
}
