package com.mapswithme.maps.news;

import androidx.annotation.DrawableRes;
import androidx.annotation.StringRes;
import com.mapswithme.maps.R;
import com.mapswithme.util.UiUtils;

public enum WelcomeScreenBindingType
{
  DOWNLOAD_GUIDE(R.string.visible,
                 UiUtils.NO_ID,
                 R.string.view_campaign_button,
                 R.string.visible,
                 R.drawable.img_download_guide, false),
  READY_TO_USE_GUIDE(R.string.visible,
                     UiUtils.NO_ID,
                     R.string.view_campaign_button,
                     R.string.visible,
                     R.drawable.img_download_guide, false),
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
                 UiUtils.NO_ID,
                 R.string.view_campaign_button,
                 R.string.visible,
                 R.drawable.img_share_emptions, false),
  EXPERIENCE(R.string.visible,
             UiUtils.NO_ID,
             R.string.view_campaign_button,
             R.string.visible,
             R.drawable.img_experience, false),
  DREAM_AND_PLAN(R.string.visible,
                 UiUtils.NO_ID,
                 R.string.view_campaign_button,
                 R.string.visible,
                 R.drawable.img_dream_and_plan, false),
  PERMISSION_EXPLANATION(R.string.visible,
                         R.string.visible,
                         R.string.view_campaign_button,
                         R.string.visible,
                         R.drawable.img_permission_explanation);

  @StringRes
  private final int mAcceptButtonResId;
  @StringRes
  private final int mDeclineButtonResId;
  @StringRes
  private final int mTitle;
  @StringRes
  private final int mSubtitle;
  @DrawableRes
  private final int mImage;

  private final boolean mDeclinedButton;

  WelcomeScreenBindingType(@StringRes int acceptButtonResId, @StringRes int declineButtonResId,
                           @StringRes int title, @StringRes int subtitle, @DrawableRes int image)
  {

    this(acceptButtonResId, declineButtonResId, title, subtitle, image, true);
  }

  WelcomeScreenBindingType(@StringRes int acceptButtonResId, @StringRes int declineButtonResId,
                           @StringRes int title, @StringRes int subtitle, @DrawableRes int image,
                           boolean hasDeclinedButton)
  {
    mAcceptButtonResId = acceptButtonResId;
    mDeclineButtonResId = declineButtonResId;
    mTitle = title;
    mSubtitle = subtitle;
    mImage = image;
    mDeclinedButton = hasDeclinedButton;
  }


  @StringRes
  public int getAcceptButtonResId()
  {
    return mAcceptButtonResId;
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

  public boolean hasDeclinedButton()
  {
    return mDeclinedButton;
  }

  @StringRes
  public int getDeclinedButtonResId()
  {
    if (!hasDeclinedButton())
      throw new UnsupportedOperationException("Value : " + name());

    return mDeclineButtonResId;
  }

}
