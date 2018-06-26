package com.mapswithme.maps.adapter;

import android.content.Context;
import android.support.annotation.DrawableRes;
import android.support.annotation.NonNull;
import android.support.annotation.StringRes;

import com.mapswithme.maps.MwmActivity;

public interface BottomSheetItem
{
  boolean isSelected(@NonNull Context context);

  void onSelected(@NonNull MwmActivity activity);

  @DrawableRes
  int getDrawableResId(@NonNull Context context);

  @StringRes
  int getTitleResId();
}
