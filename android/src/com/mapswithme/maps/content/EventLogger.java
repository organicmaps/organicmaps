package com.mapswithme.maps.content;

import android.app.Activity;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import java.util.Map;

public interface EventLogger
{
  void sendTags(@NonNull String tag, @Nullable String[] params);
  void logEvent(@NonNull String event, @NonNull Map<String, String> params);
  void startActivity(@NonNull Activity context);
  void stopActivity(@NonNull Activity context);
}
