package com.mapswithme.maps.base;

import android.app.Activity;
import androidx.annotation.NonNull;
import androidx.annotation.StyleRes;

public interface BaseActivity
{
  @NonNull
  Activity get();
  @StyleRes
  int getThemeResourceId(@NonNull String theme);
}
