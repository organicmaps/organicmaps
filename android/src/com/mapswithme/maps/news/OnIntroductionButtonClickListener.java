package com.mapswithme.maps.news;

import android.app.Activity;
import androidx.annotation.NonNull;

public interface OnIntroductionButtonClickListener
{
  void onIntroductionButtonClick(@NonNull Activity activity, @NonNull String deeplink);
}
