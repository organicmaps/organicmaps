package com.mapswithme.maps.news;

import androidx.annotation.DrawableRes;
import androidx.annotation.Nullable;
import androidx.annotation.StringRes;
import com.mapswithme.maps.R;

public enum WelcomeScreenBindingType
{
  DOWNLOAD_GUIDE(R.string.visible, R.string.visible, R.string.view_campaign_button,
                 R.string.visible,
                 R.drawable.img_download_guide),
  CHECK_OUT_SIGHTS(R.string.visible, R.string.visible, R.string.view_campaign_button,
                   R.string.visible,
                   R.drawable.img_check_sights_out),
  SUBSCRIBE_TO_CATALOG(R.string.visible, R.string.visible, R.string.view_campaign_button,
                       R.string.visible,
                       R.drawable.img_discover_guides),
  DISCOVER_GUIDES(R.string.visible, R.string.visible, R.string.view_campaign_button,
                  R.string.visible,
                  R.drawable.img_discover_guides),
  SHARE_EMOTIONS(R.string.visible, R.string.visible, R.string.view_campaign_button,
                 R.string.visible,
                 R.drawable.img_share_emptions),
  EXPERIENCE(R.string.visible, R.string.visible, R.string.view_campaign_button, R.string.visible,
             R.drawable.img_experience),
  DREAM_AND_PLAN(R.string.visible, null, R.string.view_campaign_button, null,
                 R.drawable.img_dream_and_plan),
  PERMISSION_EXPLANATION(R.string.visible, R.string.visible, R.string.view_campaign_button, null,
                         R.drawable.img_permission_explanation);

  @StringRes
  private final int mFirstButton;
  @Nullable
  private final Integer mSecondButton;
  @StringRes
  private final int mTitle;
  @Nullable
  private final Integer mSubtitle;
  @DrawableRes
  private final int mImage;

  WelcomeScreenBindingType(@StringRes int firstButton, @Nullable Integer secondButton,
                           @StringRes int title, @Nullable Integer subtitle, @DrawableRes int image)
  {
    mFirstButton = firstButton;
    mSecondButton = secondButton;
    mTitle = title;
    mSubtitle = subtitle;
    mImage = image;
  }

  @StringRes
  public int getAcceptButton()
  {
    return mFirstButton;
  }

  @Nullable
  public Integer getDeclineButton()
  {
    return mSecondButton;
  }

  @StringRes
  public int getTitle()
  {
    return mTitle;
  }

  @Nullable
  public Integer getSubtitle()
  {
    return mSubtitle;
  }

  public int getImage()
  {
    return mImage;
  }
}
