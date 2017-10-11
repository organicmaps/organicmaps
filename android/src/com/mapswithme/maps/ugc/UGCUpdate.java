package com.mapswithme.maps.ugc;

import android.support.annotation.Nullable;

class UGCUpdate
{
  @Nullable
  private final UGC.Rating[] mRatings;
  @Nullable
  private String mText;
  private long mTimeMillis;

  UGCUpdate(@Nullable UGC.Rating[] ratings, @Nullable String text, long timeMillis)
  {
    mRatings = ratings;
    mText = text;
    mTimeMillis = timeMillis;
  }

  public void setText(@Nullable String text)
  {
    mText = text;
  }

  @Nullable
  public String getText()
  {
    return mText;
  }

  long getTimeMillis()
  {
    return mTimeMillis;
  }
}
