package com.mapswithme.maps.purchase;

import androidx.annotation.StringRes;
import com.mapswithme.maps.R;

public enum BookmarksAllSubscriptionPage
{
  FIRST(R.string.all_pass_subscription_message_title,
        R.string.all_pass_subscription_message_subtitle),
  SECOND(R.string.all_pass_subscription_message_title_3,
         R.string.all_pass_subscription_message_subtitle_3),
  THIRD(R.string.all_pass_subscription_message_title_2,
        R.string.all_pass_subscription_message_subtitle_2);

  @StringRes
  private final int mTitleId;
  @StringRes
  private final int mDescriptionId;

  BookmarksAllSubscriptionPage(@StringRes int titleId, @StringRes int descriptionId)
  {
    mTitleId = titleId;
    mDescriptionId = descriptionId;
  }

  @StringRes
  public int getTitleId()
  {
    return mTitleId;
  }

  @StringRes
  public int getDescriptionId()
  {
    return mDescriptionId;
  }
}
