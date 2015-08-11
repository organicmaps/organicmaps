package com.mapswithme.maps;

import android.content.Intent;

public interface IntentProcessor
{
  boolean isIntentSupported(Intent intent);

  boolean processIntent(Intent intent);
}
