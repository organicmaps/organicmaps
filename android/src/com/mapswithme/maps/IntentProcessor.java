package com.mapswithme.maps;

import android.content.Intent;

public interface IntentProcessor
{
  boolean isSupported(Intent intent);

  boolean process(Intent intent);
}
