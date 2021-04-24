package com.mapswithme.maps.background;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import androidx.annotation.NonNull;

import android.text.TextUtils;
import android.util.Log;

import com.mapswithme.util.CrashlyticsUtils;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

import static com.mapswithme.maps.MwmApplication.backgroundTracker;

public abstract class AbstractLogBroadcastReceiver extends BroadcastReceiver
{
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);

  @Override
  public final void onReceive(Context context, Intent intent)
  {
    if (intent == null)
    {
      LOGGER.w(getTag(), "A null intent detected");
      return;
    }

    String action = intent.getAction();
    if (!TextUtils.equals(getAssertAction(), action))
    {
      LOGGER.w(getTag(), "An intent with wrong action detected: " + action);
      return;
    }

    String msg = "onReceive: " + intent + " app in background = "
                 + !backgroundTracker(context).isForeground();
    LOGGER.i(getTag(), msg);
    CrashlyticsUtils.INSTANCE.log(Log.INFO, getTag(), msg);
    onReceiveInternal(context, intent);
  }

  @NonNull
  protected String getTag()
  {
    return getClass().getSimpleName();
  }

  @NonNull
  protected abstract String getAssertAction();

  @SuppressWarnings("unused")
  public abstract void onReceiveInternal(@NonNull Context context, @NonNull Intent intent);
}
