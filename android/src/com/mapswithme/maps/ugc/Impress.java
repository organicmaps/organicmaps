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
  COMING_SOON(R.drawable.ic_24px_rating_coming_soon, R.color.rating_coming_soon),
  POPULAR(R.drawable.ic_thumb_up, R.color.rating_coming_soon),
  DISCOUNT(R.drawable.ic_thumb_up, android.R.color.white, R.color.rating_coming_soon);

  @DrawableRes
  private final int mDrawableId;
  @ColorRes
  private final int mTextColor;
  @ColorRes
  private final int mBgColor;

  Impress(@DrawableRes int smile, @ColorRes int color)
  {
    this(smile, color, color);
  }

  Impress(@DrawableRes int smile, @ColorRes int color, @ColorRes int bgColor)
  {
    mDrawableId = smile;
    mTextColor = color;
    mBgColor = bgColor;
  }

  @ColorRes
  public int getTextColor()
  {
    return mTextColor;
  }

  @DrawableRes
  public int getDrawableId()
  {
    return mDrawableId;
  }

  @ColorRes
  public int getBgColor()
  {
    return mBgColor;
  }
}
