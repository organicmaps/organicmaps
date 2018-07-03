package com.mapswithme.maps;

import android.content.Intent;
import android.support.annotation.NonNull;

public interface IntentProcessor
{
  boolean isSupported(@NonNull Intent intent);

  boolean process(@NonNull Intent intent);
}

