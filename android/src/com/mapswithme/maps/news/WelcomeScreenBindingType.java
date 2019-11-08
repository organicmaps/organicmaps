package com.mapswithme.maps.news;

import androidx.annotation.DrawableRes;
import androidx.annotation.Nullable;
import androidx.annotation.StringRes;
import com.mapswithme.maps.R;

public enum WelcomeScreenBindingType
{
  READY_TO_USE_GUIDE(R.string.visible,
                     null,
                     R.string.view_campaign_button,
                     R.string.visible,
                     R.drawable.img_download_guide),
  DOWNLOAD_GUIDE(R.string.visible,
                 null,
                 R.string.view_campaign_button,
                 R.string.visible,
                 R.drawable.img_download_guide),
  CHECK_OUT_SIGHTS(R.string.visible,
                   R.string.visible,
                   R.string.view_campaign_button,
                   R.string.visible,
                   R.drawable.img_check_sights_out),
  SUBSCRIBE_TO_CATALOG(R.string.visible,
                       R.string.visible,
                       R.string.view_campaign_button,
                       R.string.visible,
                       R.drawable.img_discover_guides),
  DISCOVER_GUIDES(R.string.visible,
                  R.string.visible,
                  R.string.view_campaign_button,
                  R.string.visible,
                  R.drawable.img_discover_guides),
  SHARE_EMOTIONS(R.string.visible,
                 null,
                 R.string.view_campaign_button,
                 R.string.visible,
                 R.drawable.img_share_emptions),
  EXPERIENCE(R.string.visible,
             null,
             R.string.view_campaign_button,
             R.string.visible,
             R.drawable.img_experience),
  DREAM_AND_PLAN(R.string.visible,
                 null,
                 R.string.view_campaign_button,
                 R.string.visible,
                 R.drawable.img_dream_and_plan),
  PERMISSION_EXPLANATION(R.string.visible,
                         R.string.visible,
                         R.string.view_campaign_button,
                         R.string.visible,
                         R.drawable.img_permission_explanation);

  @StringRes
  private final int mAcceptButton;
  @Nullable
  private final Integer mDeclineButton;
  @StringRes
  private final int mTitle;
  @StringRes
  private final int mSubtitle;
  @DrawableRes
  private final int mImage;

  WelcomeScreenBindingType(@StringRes int acceptButton, @Nullable Integer declineButton,
                           @StringRes int title, @StringRes int subtitle, @DrawableRes int image)
  {
    mAcceptButton = acceptButton;
    mDeclineButton = declineButton;
    mTitle = title;
    mSubtitle = subtitle;
    mImage = image;
  }

  @StringRes
  public int getAcceptButton()
  {
    return mAcceptButton;
  }

  @Nullable
  public Integer getDeclineButton()
  {
    return mDeclineButton;
  }

  @StringRes
  public int getTitle()
  {
    return mTitle;
  }

  @StringRes
  public int getSubtitle()
  {
    return mSubtitle;
  }

  @DrawableRes
  public int getImage()
  {
    return mImage;
  }
}
