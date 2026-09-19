package app.organicmaps.search;

import android.content.Context;
import android.content.SharedPreferences;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import app.organicmaps.sdk.Framework;
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.HashSet;
import java.util.Locale;
import java.util.Set;

final class ContactLocationCache
{
  interface MapVersionProvider
  {
    long get(double lat, double lon);
  }

  static final class Entry
  {
    final double lat;
    final double lon;
    final boolean estimated;

    Entry(double lat, double lon, boolean estimated)
    {
      this.lat = lat;
      this.lon = lon;
      this.estimated = estimated;
    }
  }

  private static final String PREFS_NAME = "contact_location_cache";
  private static final String VALUE_SEPARATOR = "\\|";
  // Increment when address resolution changes so cached contact marks are recomputed with the
  // same estimator used by interactive search.
  private static final int FORMAT_VERSION = 2;

  @NonNull
  private final SharedPreferences mPreferences;
  @NonNull
  private final MapVersionProvider mMapVersionProvider;

  ContactLocationCache(@NonNull Context context)
  {
    this(context, Framework::nativeGetMwmVersion);
  }

  ContactLocationCache(@NonNull Context context, @NonNull MapVersionProvider mapVersionProvider)
  {
    mPreferences = context.getApplicationContext().getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE);
    mMapVersionProvider = mapVersionProvider;
  }

  @Nullable
  Entry get(@NonNull String addressKey)
  {
    final String storageKey = hash(addressKey);
    final String value = mPreferences.getString(storageKey, null);
    if (value == null)
      return null;

    final String[] fields = value.split(VALUE_SEPARATOR, -1);
    if (fields.length != 5)
      return remove(storageKey);
    try
    {
      if (Integer.parseInt(fields[0]) != FORMAT_VERSION)
        return remove(storageKey);
      final long version = Long.parseLong(fields[1]);
      final double lat = Double.parseDouble(fields[2]);
      final double lon = Double.parseDouble(fields[3]);
      if (version <= 0 || mMapVersionProvider.get(lat, lon) != version)
        return remove(storageKey);
      return new Entry(lat, lon, Boolean.parseBoolean(fields[4]));
    }
    catch (NumberFormatException ignored)
    {
      return remove(storageKey);
    }
  }

  void put(@NonNull String addressKey, @NonNull Entry entry)
  {
    final long version = mMapVersionProvider.get(entry.lat, entry.lon);
    if (version <= 0)
      return;
    final String value = String.format(Locale.ROOT, "%d|%d|%.5f|%.5f|%b", FORMAT_VERSION, version, entry.lat,
                                       entry.lon, entry.estimated);
    mPreferences.edit().putString(hash(addressKey), value).apply();
  }

  void retain(@NonNull Set<String> addressKeys)
  {
    final Set<String> retained = new HashSet<>();
    for (String key : addressKeys)
      retained.add(hash(key));
    final SharedPreferences.Editor editor = mPreferences.edit();
    for (String key : mPreferences.getAll().keySet())
    {
      if (!retained.contains(key))
        editor.remove(key);
    }
    editor.apply();
  }

  void clear()
  {
    mPreferences.edit().clear().apply();
  }

  @Nullable
  private Entry remove(@NonNull String storageKey)
  {
    mPreferences.edit().remove(storageKey).apply();
    return null;
  }

  @NonNull
  static String hash(@NonNull String value)
  {
    try
    {
      final byte[] digest = MessageDigest.getInstance("SHA-256").digest(value.getBytes(StandardCharsets.UTF_8));
      final StringBuilder result = new StringBuilder(digest.length * 2);
      for (byte item : digest)
        result.append(String.format(Locale.ROOT, "%02x", item & 0xff));
      return result.toString();
    }
    catch (NoSuchAlgorithmException exception)
    {
      throw new AssertionError(exception);
    }
  }
}
