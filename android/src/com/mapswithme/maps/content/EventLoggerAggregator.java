package com.mapswithme.maps.content;

import android.app.Activity;
import android.app.Application;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import java.util.HashMap;
import java.util.Map;

public class EventLoggerAggregator extends ContextDependentEventLogger
{
  @NonNull
  private final Map<Class<? extends EventLogger>, EventLogger> mLoggers;

  public EventLoggerAggregator(@NonNull Application application)
  {
    super(application);
    mLoggers = new HashMap<>();
    mLoggers.put(LibnotifyEventLogger.class, new LibnotifyEventLogger(application));
    mLoggers.put(PushWooshEventLogger.class, new PushWooshEventLogger(application));
    mLoggers.put(MyTrackerEventLogger.class, new MyTrackerEventLogger(application));
    mLoggers.put(FlurryEventLogger.class, new FlurryEventLogger(application));
  }

  @Override
  public void sendTags(@NonNull String tag, @Nullable String[] params)
  {
    for (Map.Entry<Class<? extends EventLogger>, ? extends EventLogger> each : mLoggers.entrySet())
    {
      each.getValue().sendTags(tag, params);
    }
  }

  @Override
  public void logEvent(@NonNull String event, @NonNull Map<String, String> params)
  {
    for (Map.Entry<Class<? extends EventLogger>, ? extends EventLogger> each : mLoggers.entrySet())
    {
      each.getValue().logEvent(event, params);
    }
  }

  @Override
  public void startActivity(@NonNull Activity context)
  {
    for (Map.Entry<Class<? extends EventLogger>, ? extends EventLogger> each : mLoggers.entrySet())
    {
      each.getValue().stopActivity(context);
    }
  }

  @Override
  public void stopActivity(@NonNull Activity context)
  {
    for (Map.Entry<Class<? extends EventLogger>, ? extends EventLogger> each : mLoggers.entrySet())
    {
      each.getValue().stopActivity(context);
    }
  }
}
