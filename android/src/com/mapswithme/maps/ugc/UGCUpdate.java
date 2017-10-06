package com.mapswithme.maps.ugc;

import android.os.Parcel;
import android.os.Parcelable;
import android.support.annotation.Nullable;

import java.util.Arrays;
import java.util.Collections;
import java.util.List;

public class UGCUpdate
{
  @Nullable
  private final UGC.Rating[] mRatings;
  @Nullable
  private String mText;

  public UGCUpdate(@Nullable UGC.Rating[] ratings, @Nullable String text)
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

  @Nullable
  public List<UGC.Rating> getRatings()
  {
    if (mRatings == null)
      return null;

    return Collections.synchronizedList(Arrays.asList(mRatings));
  }
}
