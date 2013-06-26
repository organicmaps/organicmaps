package com.mapswithme.util;

import java.io.Closeable;
import java.io.IOException;
import java.io.InputStream;
import java.util.Map;
import java.util.Scanner;

import android.app.Activity;
import android.content.ClipData;
import android.content.Context;
import android.content.Intent;
import android.os.Build;
import android.util.Log;
import android.view.Window;
import android.widget.EditText;
import android.widget.Toast;

final public class Utils
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

  public static String readStreamAsString(InputStream is)
  {
    Scanner scanner = new Scanner(is).useDelimiter("\\A");
    return scanner.hasNext() ? scanner.next() : "";
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

  public static void setStringAndCursorToEnd(EditText edit, String s)
  {
    edit.setText(s);
    edit.setSelection(s.length());
  }

  public static void toastShortcut(Context context, String message)
  {
    Toast.makeText(context, message, Toast.LENGTH_LONG).show();
  }

  public static void toastShortcut(Context context, int messageResId)
  {
    final String message = context.getString(messageResId);
    toastShortcut(context, message);
  }

  public static boolean isIntentSupported(Context context, Intent intent)
  {
    return context.getPackageManager().resolveActivity(intent, 0) != null;
  }

  public static boolean apiEqualOrGreaterThan(int api)
  {
    return Build.VERSION.SDK_INT >= api;
  }

  public static boolean apiLowerThan(int api)
  {
    return Build.VERSION.SDK_INT < api;
  }

  public static void checkNotNull(Object object)
  {
    if (null == object) throw new NullPointerException("Argument here must not be NULL");
  }

  @SuppressWarnings("deprecation")
  public static void copyTextToClipboard(Context context, String text)
  {
    if (apiLowerThan(11))
    {
      android.text.ClipboardManager clipbord =
          (android.text.ClipboardManager) context.getSystemService(Context.CLIPBOARD_SERVICE);
      clipbord.setText(text);
    }
    else
    {
      // This is different classes in different packages
      android.content.ClipboardManager clipboard =
           (android.content.ClipboardManager) context.getSystemService(Context.CLIPBOARD_SERVICE);
      ClipData clip = ClipData.newPlainText("MapsWithMe: " + text, text);
      clipboard.setPrimaryClip(clip);
    }
  }

  public static <K,V> String mapPrettyPrint(Map<K, V> map)
  {
    if (map == null)   return "[null]";
    if (map.isEmpty()) return "[]";

    StringBuilder sb = new StringBuilder("[");
    for (K key : map.keySet())
      sb.append(String.valueOf(key))
        .append('=')
        .append(map.get(key))
        .append(", ");

    // Replace last ', ' with ']'.
    sb.replace(sb.length() - 2, sb.length() - 1, "]");
    return sb.toString();
  }

  // utility class
  private Utils() {};
}
