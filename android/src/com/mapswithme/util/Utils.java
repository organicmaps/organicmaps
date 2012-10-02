package com.mapswithme.util;

import java.io.Closeable;
import java.io.IOException;

import android.app.Activity;
import android.util.Log;
import android.view.Window;

public class Utils
{
  private static final String TAG = "Utils";

  public static void closeStream(Closeable stream)
  {
    if (stream != null)
    {
      try
      {
        stream.close();
      }
      catch (IOException e)
      {
        Log.e(TAG, "Can't close stream", e);
      }
    }
  }

  public static boolean isKindleFire()
  {
    return android.os.Build.MODEL.equals("Kindle Fire");
  }

  // if enabled, screen will be turned off automatically by the system
  // if disabled, screen will be always turn on
  public static void automaticIdleScreen(boolean enable, Window w)
  {
    if (enable)
      w.clearFlags(android.view.WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
    else
      w.addFlags(android.view.WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
  }

  public static float getAttributeDimension(Activity activity, int attr)
  {
    android.util.TypedValue value = new android.util.TypedValue();
    boolean b = activity.getTheme().resolveAttribute(attr, value, true);
    assert(b);
    android.util.DisplayMetrics metrics = new android.util.DisplayMetrics();
    activity.getWindowManager().getDefaultDisplay().getMetrics(metrics);
    return value.getDimension(metrics);
  }
}
