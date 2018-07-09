package com.mapswithme.maps.ugc;

import android.support.annotation.ColorRes;
import android.support.annotation.DrawableRes;

import com.mapswithme.maps.R;

public enum Impress
{
  NONE(R.drawable.ic_24px_rating_normal, R.color.rating_none),
  HORRIBLE(R.drawable.ic_24px_rating_horrible, R.color.rating_horrible),
  BAD(R.drawable.ic_24px_rating_bad, R.color.rating_bad),
  NORMAL(R.drawable.ic_24px_rating_normal, R.color.rating_normal),
  GOOD(R.drawable.ic_24px_rating_good, R.color.rating_good),
  EXCELLENT(R.drawable.ic_24px_rating_excellent, R.color.rating_excellent),
  COMING_SOON(R.drawable.ic_24px_rating_coming_soon, R.color.rating_coming_soon);

  @DrawableRes
  private final int mDrawableId;
  @ColorRes
  private final int mColorId;

  Impress(@DrawableRes int smile, @ColorRes int color)
  {
    mDrawableId = smile;
    mColorId = color;
  }

  @ColorRes
  public int getColorId()
  {
    return mColorId;
  }

  @DrawableRes
  public int getDrawableId()
  {
    return mDrawableId;
  }
}
