package com.mapswithme.maps.onboarding;

import androidx.annotation.NonNull;
import androidx.fragment.app.FragmentActivity;

public interface OnboardingScreenLauncher
{
  void launchScreen(@NonNull FragmentActivity activity, @NonNull String url);
}
