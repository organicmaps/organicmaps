package com.mapswithme.maps.intent;

import android.content.Intent;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import com.mapswithme.maps.MwmActivity;

public interface IntentProcessor
{
  boolean isSupported(@NonNull Intent intent);

  @Nullable
  MwmActivity.MapTask process(@NonNull Intent intent);
}

