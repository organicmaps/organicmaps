package com.mapswithme.util;

import java.io.Closeable;
import java.io.IOException;
import java.io.InputStream;
import java.util.List;
import java.util.Map;
import java.util.Scanner;
import com.mapswithme.maps.MWMApplication;
import android.annotation.SuppressLint;
import android.annotation.TargetApi;
import android.app.Activity;
import android.content.ClipData;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.os.Build;
import android.text.TextUtils;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
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
      catch (final IOException e)
      {
        Log.e(TAG, "Can't close stream", e);
      }
    }
  }

  public static String readStreamAsString(InputStream is)
  {
    final Scanner scanner = new Scanner(is).useDelimiter("\\A");
    return scanner.hasNext() ? scanner.next() : "";
  }

  public static boolean isAmazonDevice()
  {
    final boolean byAmazon = "Amazon".equalsIgnoreCase(Build.MANUFACTURER);
    return byAmazon;
  }


  public static boolean hasAnyGoogleStoreInstalled()
  {
    return hasAnyGoogleStoreInstalled(MWMApplication.get());
  }

  public static boolean hasAnyGoogleStoreInstalled(Context context)
  {
    final String GooglePlayStorePackageNameOld = "com.google.market";
    final String GooglePlayStorePackageNameNew = "com.android.vending";
    final PackageManager pm = context.getPackageManager();
    final List<PackageInfo> packages = pm.getInstalledPackages(0);
    for (final PackageInfo packageInfo : packages)
    {
      if (packageInfo.packageName.equals(GooglePlayStorePackageNameOld)
          || packageInfo.packageName.equals(GooglePlayStorePackageNameNew))
        return true;
    }
    return false;
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
    final android.util.TypedValue value = new android.util.TypedValue();
    final boolean b = activity.getTheme().resolveAttribute(attr, value, true);
    assert(b);
    final android.util.DisplayMetrics metrics = new android.util.DisplayMetrics();
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

  @SuppressLint("NewApi")
  @SuppressWarnings("deprecation")
  public static void copyTextToClipboard(Context context, String text)
  {
    if (apiLowerThan(11))
    {
      final android.text.ClipboardManager clipbord =
          (android.text.ClipboardManager) context.getSystemService(Context.CLIPBOARD_SERVICE);
      clipbord.setText(text);
    }
    else
    {
      // This is different classes in different packages
      final android.content.ClipboardManager clipboard =
           (android.content.ClipboardManager) context.getSystemService(Context.CLIPBOARD_SERVICE);
      final ClipData clip = ClipData.newPlainText("MapsWithMe: " + text, text);
      clipboard.setPrimaryClip(clip);
    }
  }

  public static <K,V> String mapPrettyPrint(Map<K, V> map)
  {
    if (map == null)
      return "[null]";
    if (map.isEmpty())
      return "[]";


    String joined = "";
    for (final K key : map.keySet())
    {
      final String keyVal =  key + "=" + map.get(key);
      if (joined.length() > 0)
        joined = TextUtils.join(",", new Object[] { joined, keyVal });
      else
        joined = keyVal;
    }

    return "[" + joined + "]";
  }

  @TargetApi(Build.VERSION_CODES.HONEYCOMB)
  public static MenuItem addMenuCompat(Menu menu, int id, int order, int titleResId, int iconResId)
  {
    final MenuItem mItem = menu.add(Menu.NONE, id, order, titleResId);
    mItem.setIcon(iconResId);
    if (apiEqualOrGreaterThan(11))
      mItem.setShowAsAction(MenuItem.SHOW_AS_ACTION_IF_ROOM);

    return mItem;
  }

  // utility class
  private Utils() {};
}
