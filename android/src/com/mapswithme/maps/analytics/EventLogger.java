package com.mapswithme.maps.analytics;

import android.app.Activity;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import java.util.Map;

public interface EventLogger
{
  void initialize();
  void sendTags(@NonNull String tag, @Nullable String[] params);
  void logEvent(@NonNull String event, @NonNull Map<String, String> params);
  void startActivity(@NonNull Activity context);
  void stopActivity(@NonNull Activity context);
}
