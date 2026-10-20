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

    Entry(double lat, double lon)
    {
      this.lat = lat;
      this.lon = lon;
    }
  }

  private static final String PREFS_NAME = "contact_location_cache";
  private static final String VALUE_SEPARATOR = "\\|";
  // Increment when address resolution changes so cached contact marks are recomputed with the
  // same mapped-address resolver used by contact-address resolution.
  private static final int FORMAT_VERSION = 9;
  private static final String UNRESOLVED_PREFIX = "unresolved:";
  // Bump when address resolution semantics change; old misses must not suppress improved matching.
  private static final int UNRESOLVED_FORMAT_VERSION = 1;

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
    if (fields.length != 4)
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
      return new Entry(lat, lon);
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
    final String value = String.format(Locale.ROOT, "%d|%d|%.5f|%.5f", FORMAT_VERSION, version, entry.lat, entry.lon);
    mPreferences.edit().remove(UNRESOLVED_PREFIX + hash(addressKey)).putString(hash(addressKey), value).apply();
  }

  boolean isUnresolved(@NonNull String addressKey, @NonNull String context)
  {
    final String key = UNRESOLVED_PREFIX + hash(addressKey);
    final String value = mPreferences.getString(key, null);
    if (value == null)
      return false;
    if (!context.isEmpty() && value.equals(UNRESOLVED_FORMAT_VERSION + "|" + context))
      return true;
    mPreferences.edit().remove(key).apply();
    return false;
  }

  void putUnresolved(@NonNull String addressKey, @NonNull String context)
  {
    if (!context.isEmpty())
      mPreferences.edit()
          .putString(UNRESOLVED_PREFIX + hash(addressKey), UNRESOLVED_FORMAT_VERSION + "|" + context)
          .apply();
  }

  void retain(@NonNull Set<String> addressKeys)
  {
    final Set<String> retained = new HashSet<>();
    for (String key : addressKeys)
    {
      retained.add(hash(key));
      retained.add(UNRESOLVED_PREFIX + hash(key));
    }
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

  void clearUnresolved()
  {
    final SharedPreferences.Editor editor = mPreferences.edit();
    for (String key : mPreferences.getAll().keySet())
      if (key.startsWith(UNRESOLVED_PREFIX))
        editor.remove(key);
    editor.apply();
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
      final char[] result = new char[digest.length * 2];
      final String hex = "0123456789abcdef";
      for (int i = 0; i < digest.length; ++i)
      {
        final int valueByte = digest[i] & 0xff;
        result[2 * i] = hex.charAt(valueByte >>> 4);
        result[2 * i + 1] = hex.charAt(valueByte & 0xf);
      }
      return new String(result);
    }
    catch (NoSuchAlgorithmException exception)
    {
      throw new AssertionError(exception);
    }
  }
}
