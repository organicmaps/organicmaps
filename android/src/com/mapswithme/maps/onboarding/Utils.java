package com.mapswithme.maps.onboarding;

import androidx.annotation.NonNull;
import com.mapswithme.maps.news.OnboardingStep;

public class Utils
{
  @NonNull
  public static OnboardingStep getOnboardingStepByTip(@NonNull OnboardingTip tip)
  {
    switch (tip.getType())
    {
      case OnboardingTip.BUY_SUBSCRIPTION:
        return OnboardingStep.SUBSCRIBE_TO_CATALOG;
      case OnboardingTip.DISCOVER_CATALOG:
        return OnboardingStep.DISCOVER_GUIDES;
      case OnboardingTip.DOWNLOAD_SAMPLES:
        return OnboardingStep.CHECK_OUT_SIGHTS;
      default:
        throw new UnsupportedOperationException("Unsupported onboarding tip: " + tip);
    }
  }
}
