package com.mapswithme.maps;

import android.content.Intent;

public interface IntentProcessor {

  public boolean isIntentSupported(Intent intent);

  public boolean processIntent(Intent intent);

}
