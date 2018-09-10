package com.mapswithme.maps.tips;

import android.support.annotation.NonNull;

import com.mapswithme.maps.MwmActivity;

public interface ClickInterceptor
{
  void onInterceptClick(@NonNull MwmActivity activity);
}
