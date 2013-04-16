package com.mapswithme.util;

import java.io.Closeable;
import java.io.IOException;

import android.app.Activity;
import android.content.Context;
import android.util.Log;
import android.view.Window;
import android.widget.EditText;
import android.widget.Toast;
import android.os.Build;

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

  public static boolean isAmazonDevice()
  {
    final boolean byAmazon = "Amazon".equalsIgnoreCase(Build.MANUFACTURER);
    return byAmazon;
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

  /*
  public static String toTitleCase(String str)
  {
    if (str == null)
      return null;

    boolean space = true;
    StringBuilder builder = new StringBuilder(str);

    final int len = builder.length();
    for (int i = 0; i < len; ++i)
    {
      char c = builder.charAt(i);
      if (space)
      {
        if (!Character.isWhitespace(c))
        {
          // Convert to title case and switch out of whitespace mode.
          builder.setCharAt(i, Character.toTitleCase(c));
          space = false;
        }
      }
      else if (Character.isWhitespace(c))
      {
        space = true;
      }
      else
      {
        builder.setCharAt(i, Character.toLowerCase(c));
      }
    }

    return builder.toString();
  }
   */

  public static void setStringAndCursorToEnd(EditText edit, String s)
  {
    edit.setText(s);
    edit.setSelection(s.length());
  }

  public static void toastShortcut(Context context, String message) {
    Toast.makeText(context, message, Toast.LENGTH_LONG).show();
  }

  public static void toastShortcut(Context context, int messageResId) {
    final String message = context.getString(messageResId);
    toastShortcut(context, message);
  }

}
