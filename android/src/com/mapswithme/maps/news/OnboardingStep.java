package com.mapswithme.maps.news;

import androidx.annotation.DrawableRes;
import androidx.annotation.NonNull;
import androidx.annotation.StringRes;
import com.mapswithme.maps.R;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.statistics.StatisticValueConverter;

public enum OnboardingStep implements StatisticValueConverter<String>
{
  CHECK_OUT_SIGHTS(R.string.new_onboarding_step5_3_button,
                   R.string.later,
                   R.string.new_onboarding_step5_1_header,
                   R.string.new_onboarding_step5_3_message)
      {
        @NonNull
        @Override
        public String toStatisticValue()
        {
          return "sample_discovery";
        }
      },
  SUBSCRIBE_TO_CATALOG(R.string.new_onboarding_step5_2_button,
                       R.string.later,
                       R.string.new_onboarding_step5_1_header,
                       R.string.new_onboarding_step5_2_message)
      {
        @NonNull
        @Override
        public String toStatisticValue()
        {
          return "buy_subscription";
        }
      },
  DISCOVER_GUIDES(R.string.new_onboarding_step5_1_button,
                  R.string.later,
                  R.string.new_onboarding_step5_1_header,
                  R.string.new_onboarding_step5_1_message)
      {
        @NonNull
        @Override
        public String toStatisticValue()
        {
          return "catalog_discovery";
        }
      },
  SHARE_EMOTIONS(R.string.new_onboarding_button_2,
                 UiUtils.NO_ID,
                 R.string.new_onboarding_step4_header,
                 R.string.new_onboarding_step4_message,
               false)
      {
        @NonNull
        @Override
        public String toStatisticValue()
        {
          return "share_emotions";
        }
      },
  EXPERIENCE(R.string.new_onboarding_button,
             UiUtils.NO_ID,
             R.string.new_onboarding_step3_header,
             R.string.new_onboarding_step3_message,
         false)
      {
        @NonNull
        @Override
        public String toStatisticValue()
        {
          return "experience";
        }
      },
  DREAM_AND_PLAN(R.string.new_onboarding_button,
                 UiUtils.NO_ID,
                 R.string.new_onboarding_step2_header,
                 R.string.new_onboarding_step2_message,
          false)
      {
        @NonNull
        @Override
        public String toStatisticValue()
        {
          return "dream_and_plan";
        }
      },
  PERMISSION_EXPLANATION(R.string.new_onboarding_button,
                         R.string.learn_more,
                         R.string.onboarding_permissions_title,
                         R.string.onboarding_permissions_message)
      {
        @NonNull
        @Override
        public String toStatisticValue()
        {
          return "permissions";
        }
      };

  @StringRes
  private final int mAcceptButtonResId;
  @StringRes
  private final int mDeclineButtonResId;
  @StringRes
  private final int mTitle;
  @StringRes
  private final int mSubtitle;


  private final boolean mDeclinedButton;

  OnboardingStep(@StringRes int acceptButtonResId, @StringRes int declineButtonResId,
                 @StringRes int title, @StringRes int subtitle)
  {

    this(acceptButtonResId, declineButtonResId, title, subtitle,true);
  }

  OnboardingStep(@StringRes int acceptButtonResId, @StringRes int declineButtonResId,
                 @StringRes int title, @StringRes int subtitle,
                 boolean hasDeclinedButton)
  {
    mAcceptButtonResId = acceptButtonResId;
    mDeclineButtonResId = declineButtonResId;
    mTitle = title;
    mSubtitle = subtitle;
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
    return 0;
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
