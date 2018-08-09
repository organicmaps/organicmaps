package com.mapswithme.maps.intent;

import android.content.Intent;
import android.support.annotation.NonNull;

import com.mapswithme.maps.MwmActivity;

public interface IntentProcessor
{
  boolean isSupported(@NonNull Intent intent);

  @NonNull
  MwmActivity.MapTask process(@NonNull Intent intent);
}

