package app.organicmaps.search;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertNull;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.content.Context;
import android.content.SharedPreferences;
import java.util.Map;
import java.util.Set;
import org.junit.Test;

public class ContactLocationCacheTest
{
  @Test
  public void hashesAddressesDeterministicallyWithoutStoringAddressText()
  {
    final String address = "11378 158A Street Surrey";
    final String hash = ContactLocationCache.hash(address);

    assertEquals(hash, ContactLocationCache.hash(address));
    assertEquals(64, hash.length());
    assertNotEquals(address, hash);
  }

  @Test
  public void clearsAllPersistedLocations()
  {
    final CacheMocks mocks = new CacheMocks();

    mocks.cache.clear();

    verify(mocks.editor).clear();
    verify(mocks.editor).apply();
  }

  @Test
  public void retainsOnlyCurrentAddressKeys()
  {
    final CacheMocks mocks = new CacheMocks();
    final String retained = ContactLocationCache.hash("retained");
    final String removed = ContactLocationCache.hash("removed");
    doReturn(Map.of(retained, "value", removed, "value")).when(mocks.preferences).getAll();

    mocks.cache.retain(Set.of("retained"));

    verify(mocks.editor, never()).remove(retained);
    verify(mocks.editor).remove(removed);
    verify(mocks.editor).apply();
  }

  @Test
  public void removesLocationsFromOlderMapVersions()
  {
    final CacheMocks mocks = new CacheMocks();
    final String address = "11378 158A Street Surrey";
    final String key = ContactLocationCache.hash(address);
    when(mocks.preferences.getString(key, null)).thenReturn("2|42|49.12345|-122.98765|false");

    mocks.mapVersion = 43L;
    assertNull(mocks.cache.get(address));

    verify(mocks.editor).remove(key);
  }

  @Test
  public void storesOnlyFiveDecimalPlaces()
  {
    final CacheMocks mocks = new CacheMocks();
    final String address = "11378 158A Street Surrey";
    mocks.mapVersion = 42L;
    mocks.cache.put(address, new ContactLocationCache.Entry(49.1234567, -122.9876543, true));

    verify(mocks.editor).putString(ContactLocationCache.hash(address), "2|42|49.12346|-122.98765|true");
  }

  private static final class CacheMocks
  {
    final Context context = mock(Context.class);
    final SharedPreferences preferences = mock(SharedPreferences.class);
    final SharedPreferences.Editor editor = mock(SharedPreferences.Editor.class);
    final ContactLocationCache cache;
    long mapVersion;

    CacheMocks()
    {
      when(context.getApplicationContext()).thenReturn(context);
      when(context.getSharedPreferences(anyString(), org.mockito.ArgumentMatchers.anyInt())).thenReturn(preferences);
      when(preferences.edit()).thenReturn(editor);
      when(editor.clear()).thenReturn(editor);
      when(editor.remove(anyString())).thenReturn(editor);
      when(editor.putString(anyString(), anyString())).thenReturn(editor);
      cache = new ContactLocationCache(context, (lat, lon) -> mapVersion);
    }
  }
}
