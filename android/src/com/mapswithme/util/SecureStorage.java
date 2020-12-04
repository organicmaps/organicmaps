package com.mapswithme.util;

import android.content.Context;
import android.content.SharedPreferences;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

public final class SecureStorage
{
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);
  private static final String TAG = SecureStorage.class.getSimpleName();

  private SecureStorage() {}

  public static void save(@NonNull Context context, @NonNull String key, @NonNull String value)
  {
    LOGGER.d(TAG, "save: key = " + key);
    SharedPreferences prefs = context.getSharedPreferences("secure", Context.MODE_PRIVATE);
    prefs.edit().putString(key, value).apply();
  }

  @Nullable
  public static String load(@NonNull Context context, @NonNull String key)
  {
    LOGGER.d(TAG, "load: key = " + key);
    SharedPreferences prefs = context.getSharedPreferences("secure", Context.MODE_PRIVATE);
    return prefs.getString(key, null);
  }

  public static void remove(@NonNull Context context, @NonNull String key)
  {
    LOGGER.d(TAG, "remove: key = " + key);
    SharedPreferences prefs = context.getSharedPreferences("secure", Context.MODE_PRIVATE);
    prefs.edit().remove(key).apply();
  }
}
