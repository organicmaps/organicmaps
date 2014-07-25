package com.mapswithme.maps.bookmarks.data;


public class Track
{

  //{@ Populate on create
  private final int mTrackId;
  private final int mCategoryId;
  private final String mName;
  private final String mLengthString;
  private final int mColor;
  //}@


  /* package */ Track(int trackId, int categoryId, String name, String lengthString, int color)
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

  public int getTrackId() { return mTrackId; }

  public int getCategoryId() { return mCategoryId; }
}
