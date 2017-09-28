package com.mapswithme.util;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import com.mapswithme.maps.BuildConfig;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

public final class SecureStorage
{
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);
  private static final String TAG = SecureStorage.class.getSimpleName();

  private SecureStorage() {}

  public static void save(@NonNull String key, @NonNull String value)
  {
    if (BuildConfig.DEBUG)
      LOGGER.d(TAG, "save: key = " + key + ", value = " + value);
    // TODO: implement @alexzatsepin
  }

  @Nullable
  public static String load(@NonNull String key)
  {
    if (BuildConfig.DEBUG)
      LOGGER.d(TAG, "load: key = " + key);
    // TODO: implement @alexzatsepin
    return null;
  }

  public static void remove(@NonNull String key)
  {
    if (BuildConfig.DEBUG)
      LOGGER.d(TAG, "remove: key = " + key);
    // TODO: implement @alexzatsepin
  }
}
