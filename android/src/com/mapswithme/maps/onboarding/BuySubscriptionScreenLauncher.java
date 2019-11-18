package com.mapswithme.maps.onboarding;

import android.content.Intent;

import androidx.annotation.NonNull;
import androidx.fragment.app.FragmentActivity;
import com.mapswithme.maps.purchase.BookmarksAllSubscriptionActivity;

public class BuySubscriptionScreenLauncher implements OnboardingScreenLauncher
{
  @Override
  public void launchScreen(@NonNull FragmentActivity activity, @NonNull String url)
  {
    activity.startActivity(new Intent(activity, BookmarksAllSubscriptionActivity.class));
  }
}
