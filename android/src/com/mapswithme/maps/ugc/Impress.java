package com.mapswithme.maps.ugc;

import androidx.annotation.ColorRes;
import androidx.annotation.DrawableRes;

import androidx.annotation.StringRes;
import com.mapswithme.maps.R;
import com.mapswithme.util.UiUtils;

public enum Impress
{
  NONE(R.drawable.ic_24px_rating_normal, R.color.rating_none),
  HORRIBLE(R.drawable.ic_24px_rating_horrible, R.color.rating_horrible, R.color.rating_horrible,
           R.string.placepage_rate_horrible),
  BAD(R.drawable.ic_24px_rating_bad, R.color.rating_bad, R.color.rating_bad,
      R.string.placepage_rate_bad),
  NORMAL(R.drawable.ic_24px_rating_normal, R.color.rating_normal, R.color.rating_normal,
         R.string.placepage_rate_normal),
  GOOD(R.drawable.ic_24px_rating_good, R.color.rating_good, R.color.rating_good,
       R.string.placepage_rate_good),
  EXCELLENT(R.drawable.ic_24px_rating_excellent, R.color.rating_excellent, R.color.rating_excellent,
            R.string.placepage_rate_excellent),
  COMING_SOON(R.drawable.ic_24px_rating_coming_soon, R.color.rating_coming_soon),
  POPULAR(R.drawable.ic_thumb_up, R.color.rating_coming_soon),
  DISCOUNT(R.drawable.ic_thumb_up, android.R.color.white, R.color.rating_coming_soon);

  @DrawableRes
  private final int mDrawableId;
  @ColorRes
  private final int mTextColor;
  @ColorRes
  private final int mBgColor;
  @StringRes
  private final int mTextId;

  Impress(@DrawableRes int smile, @ColorRes int color)
  {
    this(smile, color, color);
  }

  Impress(@DrawableRes int smile, @ColorRes int color, @ColorRes int bgColor)
  {
    this(smile, color, bgColor, UiUtils.NO_ID);
  }

  Impress(@DrawableRes int smile, @ColorRes int color, @ColorRes int bgColor, @StringRes int textId) {
    mDrawableId = smile;
    mTextColor = color;
    mBgColor = bgColor;
    mTextId = textId;
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

  @StringRes
  public int getTextId()
  {
    if (mTextId == UiUtils.NO_ID)
      throw new UnsupportedOperationException(String.format("Impress %s does't support textual" +
                                                            "presentation", name()));
    return mTextId;
  }
}
