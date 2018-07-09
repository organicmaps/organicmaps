package com.mapswithme.maps.background;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
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
    String action = intent != null ? intent.getAction() : null;
    if (!TextUtils.equals(getAssertAction(), action))
    {
      LOGGER.w(getTag(), "An intent with wrong action detected: " + action);
      return;
    }

    String msg = "onReceive: " + intent + " app in background = "
                 + !backgroundTracker().isForeground();
    LOGGER.i(getTag(), msg);
    CrashlyticsUtils.log(Log.INFO, getTag(), msg);
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
