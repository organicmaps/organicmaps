package com.mapswithme.maps.onboarding;

import android.app.Activity;
import androidx.annotation.NonNull;

public interface OnIntroductionButtonClickListener
{
  void onIntroductionButtonClick(@NonNull Activity activity, @NonNull String deeplink);
}
