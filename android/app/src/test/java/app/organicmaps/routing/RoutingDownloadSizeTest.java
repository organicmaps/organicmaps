package app.organicmaps.routing;

import static org.junit.Assert.assertEquals;
import static org.mockito.Mockito.CALLS_REAL_METHODS;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.when;

import android.os.Bundle;
import app.organicmaps.sdk.downloader.CountryItem;
import java.lang.reflect.Field;
import java.util.ArrayList;
import org.junit.Test;

public class RoutingDownloadSizeTest
{
  @Test
  public void recreatedDialogKeepsTheSizeCapturedBeforeEnqueueing() throws Exception
  {
    Bundle arguments = mock(Bundle.class);
    when(arguments.getStringArray(BaseRoutingErrorDialogFragment.EXTRA_MISSING_MAPS))
        .thenReturn(new String[] {"France_Nice"});
    when(arguments.containsKey("DownloadSize")).thenReturn(true);
    when(arguments.getLong("DownloadSize")).thenReturn(140000000L);
    RoutingMapsDownloadFragment fragment = mock(RoutingMapsDownloadFragment.class, CALLS_REAL_METHODS);
    when(fragment.getArguments()).thenReturn(arguments);
    Field maps = BaseRoutingErrorDialogFragment.class.getDeclaredField("mMissingMaps");
    maps.setAccessible(true);
    maps.set(fragment, new ArrayList<CountryItem>());

    try (var countries = mockStatic(CountryItem.class))
    {
      CountryItem country = new CountryItem("France_Nice");
      country.status = CountryItem.STATUS_ENQUEUED;
      country.downloadSize = 0;
      countries.when(() -> CountryItem.fill("France_Nice")).thenReturn(country);

      fragment.parseArguments();
      fragment.parseArguments();

      Field size = BaseRoutingErrorDialogFragment.class.getDeclaredField("mDownloadSize");
      size.setAccessible(true);
      assertEquals(140000000L, size.getLong(fragment));
      assertEquals(1, fragment.mMissingMaps.size());
    }
  }
}
