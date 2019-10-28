package com.mapswithme.maps.purchase;

import com.mapswithme.maps.R;

public enum AllPassSubscriptionType
{
  FIRST(R.string.all_pass_subscription_message_title,
        R.string.all_pass_subscription_message_subtitle),
  SECOND(R.string.all_pass_subscription_message_title_2,
         R.string.all_pass_subscription_message_subtitle_2),
  THIRD(R.string.all_pass_subscription_message_title_3,
        R.string.all_pass_subscription_message_subtitle_3);

  private final int mTitleId;
  private final int mDescriptionId;

  AllPassSubscriptionType(int titleId, int descriptionId)
  {
    mTitleId = titleId;
    mDescriptionId = descriptionId;
  }

  public int getTitleId()
  {
    return mTitleId;
  }

  public int getDescriptionId()
  {
    return mDescriptionId;
  }
}
