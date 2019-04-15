package com.mapswithme.maps.intent;

import android.content.Intent;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

public interface IntentProcessor
{
  boolean isSupported(@NonNull Intent intent);

  @Nullable
  MapTask process(@NonNull Intent intent);
}

