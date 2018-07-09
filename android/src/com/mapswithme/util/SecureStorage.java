package com.mapswithme.util;

import android.content.Context;
import android.content.SharedPreferences;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

public final class SecureStorage
{
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);
  private static final String TAG = SecureStorage.class.getSimpleName();
  private static final SharedPreferences mPrefs
      = MwmApplication.get().getSharedPreferences("secure", Context.MODE_PRIVATE);

  private SecureStorage() {}

  public static void save(@NonNull String key, @NonNull String value)
  {
    LOGGER.d(TAG, "save: key = " + key);
    mPrefs.edit().putString(key, value).apply();
  }

  @Nullable
  public static String load(@NonNull String key)
  {
    LOGGER.d(TAG, "load: key = " + key);
    return mPrefs.getString(key, null);
  }

  public static void remove(@NonNull String key)
  {
    LOGGER.d(TAG, "remove: key = " + key);
    mPrefs.edit().remove(key).apply();
  }
}
