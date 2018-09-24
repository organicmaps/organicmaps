package com.mapswithme.maps.content;

import android.app.Activity;
import android.app.Application;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import java.util.Map;

public class DefaultEventLogger extends ContextDependentEventLogger
{
  public DefaultEventLogger(@NonNull Application application)
  {
    super(application);
  }

  @Override
  public void sendTags(@NonNull String tag, @Nullable String[] params)
  {
    /* Do nothing */
  }

  @Override
  public void logEvent(@NonNull String event, @NonNull Map<String, String> params)
  {
    /* Do nothing */
  }

  @Override
  public void startActivity(@NonNull Activity context)
  {
    /* Do nothing */
  }

  @Override
  public void stopActivity(@NonNull Activity context)
  {
    /* Do nothing */
  }
}
