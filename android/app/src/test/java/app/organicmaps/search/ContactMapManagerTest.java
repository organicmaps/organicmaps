package app.organicmaps.search;

import static org.junit.Assert.*;
import static org.mockito.Mockito.*;

import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.os.SystemClock;
import androidx.core.content.ContextCompat;
import app.organicmaps.sdk.util.Config;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.Map;
import java.util.Set;
import org.junit.Test;
import org.mockito.MockedConstruction;
import org.mockito.MockedStatic;

public class ContactMapManagerTest
{
  @Test
  public void mapUpdateInvalidatesMemoryCoordinatesAndMarkers() throws Exception
  {
    try (MockedStatic<Looper> looper = mockStatic(Looper.class);
         MockedStatic<SystemClock> clock = mockStatic(SystemClock.class);
         MockedConstruction<Handler> handlers = mockConstruction(Handler.class);
         MockedStatic<Config> config = mockStatic(Config.class);
         MockedStatic<ContextCompat> permissions = mockStatic(ContextCompat.class))
    {
      config.when(Config::isContactSearchEnabled).thenReturn(true);
      final ContactMapManager manager = ContactMapManager.INSTANCE;
      final ContactMapManager.MarkerUpdater markers = mock(ContactMapManager.MarkerUpdater.class);
      set(manager, "mMarkerUpdater", markers);
      set(manager, "mDataVersionProvider", (ContactMapManager.DataVersionProvider) () -> "maps-v1");
      final ContactLocationCache cache = mock(ContactLocationCache.class);
      final ContactAddress contact = new ContactAddress("Name", "", "123 Main Street, Canada", "", "");
      set(manager, "mContext", mock(Context.class));
      set(manager, "mPersistentCache", cache);
      final Method viewport =
          ContactMapManager.class.getDeclaredMethod("onViewportChanged", int.class, String.class, long.class,
                                                    double.class, double.class, double.class, double.class);
      viewport.setAccessible(true);
      try
      {
        viewport.invoke(manager, 16, "Canada", 1L, 0.0, 0.0, 1.0, 1.0);
        manager.recordUnresolved(contact, false, "en");
        assertTrue(manager.hasRecentResolutionMiss(contact, "en"));
        verify(cache, never()).putUnresolved(anyString(), anyString());
        manager.recordUnresolved(contact, true, "en");
        verify(cache).putUnresolved(eq(contact.getAddressKey()), anyString());
        when(cache.get(contact.getAddressKey())).thenReturn(new ContactLocationCache.Entry(0.5, 0.5));
        manager.recordResolved(contact, 0.5, 0.5);
        assertFalse(manager.hasRecentResolutionMiss(contact, "en"));
        assertNotNull(manager.getResolved(contact));
        reset(markers);
        manager.recordUnresolved(contact, false, "en");
        assertTrue(manager.hasRecentResolutionMiss(contact, "en"));
        when(cache.get(contact.getAddressKey())).thenReturn(null);
        viewport.invoke(manager, 16, "Canada", 2L, 0.0, 0.0, 1.0, 1.0);
        verify(cache, never()).clearUnresolved();
        assertFalse(manager.hasRecentResolutionMiss(contact, "en"));
        assertNull(manager.getResolved(contact));
        verify(markers, atLeastOnce()).update(new double[0], new double[0], new String[0], new String[0]);
        set(manager, "mDataVersionProvider", (ContactMapManager.DataVersionProvider) () -> "maps-v2");
        viewport.invoke(manager, 16, "Canada", 3L, 0.0, 0.0, 1.0, 1.0);
        verify(cache).clearUnresolved();
      }
      finally
      {
        set(manager, "mContext", null);
        set(manager, "mPersistentCache", null);
        set(manager, "mViewportScale", 0);
        set(manager, "mDataVersion", "");
        set(manager, "mDataRevision", -1L);
        set(manager, "mDataVersionProvider",
            (ContactMapManager.DataVersionProvider)
                app.organicmaps.sdk.search.SearchEngine::nativeGetAddressDataVersion);
        final Field policy = ContactMapManager.class.getDeclaredField("mResolutionPolicy");
        policy.setAccessible(true);
        ((ContactResolutionPolicy) policy.get(manager)).clear();
        for (String name : new String[] {"mCache", "mNames", "mPostalAddresses"})
        {
          final Field field = ContactMapManager.class.getDeclaredField(name);
          field.setAccessible(true);
        ((Map<?, ?>) field.get(manager)).clear();
        }
        final Field visibleKeys = ContactMapManager.class.getDeclaredField("mVisibleKeys");
        visibleKeys.setAccessible(true);
        ((Set<?>) visibleKeys.get(manager)).clear();
        set(manager, "mMarkerUpdater",
            (ContactMapManager.MarkerUpdater) app.organicmaps.sdk.Framework::nativeSetContactMarks);
      }
    }
  }

  private static void set(Object object, String name, Object value) throws Exception
  {
    final Field field = object.getClass().getDeclaredField(name);
    field.setAccessible(true);
    field.set(object, value);
  }
}
