package com.mapswithme.maps.onboarding;

import androidx.annotation.NonNull;

public enum OnboardingScreen
{
  DISCOVER_CATALOG(new OpenCatalogLauncher()),
  DOWNLOAD_SAMPLES(new OpenCatalogLauncher()),
  BUY_SUBSCRIPTION(new BuySubscriptionScreenLauncher());

  @NonNull
  private final OnboardingScreenLauncher mLauncher;

  OnboardingScreen(@NonNull OnboardingScreenLauncher launcher)
  {
    mLauncher = launcher;
  }

  @NonNull
  public OnboardingScreenLauncher getOnboardingActivityLauncher()
  {
    return mLauncher;
  }
}
