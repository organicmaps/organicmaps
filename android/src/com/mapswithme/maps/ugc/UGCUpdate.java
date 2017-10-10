package com.mapswithme.maps.ugc;

import android.support.annotation.Nullable;

class UGCUpdate
{
  @Nullable
  private final UGC.Rating[] mRatings;
  @Nullable
  private String mText;

  UGCUpdate(@Nullable UGC.Rating[] ratings, @Nullable String text)
  {
    mRatings = ratings;
    mText = text;
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
}
