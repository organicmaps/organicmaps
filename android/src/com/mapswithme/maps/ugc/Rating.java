package com.mapswithme.maps.ugc;

import android.support.annotation.ColorRes;
import android.support.annotation.DrawableRes;

import com.mapswithme.maps.R;

public enum Rating
{
  EXCELLENT(R.drawable.ic_24px_rating_excellent, R.color.rating_excellent),
  GOOD(R.drawable.ic_24px_rating_good, R.color.rating_good),
  NORMAL(R.drawable.ic_24px_rating_normal, R.color.rating_normal),
  BAD(R.drawable.ic_24px_rating_bad, R.color.rating_bad),
  HORRIBLE(R.drawable.ic_24px_rating_horrible, R.color.rating_horrible);

  @DrawableRes
  private final int mDrawableId;
  @ColorRes
  private final int mColorId;

  Rating(@DrawableRes int smile, @ColorRes int color)
  {
    mDrawableId = smile;
    mColorId = color;
  }

  public int getColorId()
  {
    return mColorId;
  }

  public int getDrawableId()
  {
    return mDrawableId;
  }
}
