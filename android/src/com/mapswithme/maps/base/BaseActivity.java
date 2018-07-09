package com.mapswithme.maps.base;

import android.app.Activity;
import android.support.annotation.NonNull;
import android.support.annotation.StyleRes;

public interface BaseActivity
{
  @NonNull
  Activity get();
  @StyleRes
  int getThemeResourceId(@NonNull String theme);
}
