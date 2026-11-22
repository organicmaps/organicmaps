package app.organicmaps.car.screens.download;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.when;

import android.location.Location;
import androidx.car.app.testing.TestCarContext;
import app.organicmaps.sdk.OrganicMaps;
import app.organicmaps.sdk.downloader.CountryItem;
import app.organicmaps.sdk.downloader.MapManager;
import app.organicmaps.sdk.location.LocationHelper;
import java.util.List;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.MockedStatic;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;
import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;

@RunWith(RobolectricTestRunner.class)
@Config(shadows = {FirstLaunchDownloadTest.Sdk.class, FirstLaunchDownloadTest.Storage.class})
public class FirstLaunchDownloadTest
{
  @Test
  public void resumingReplacesTheOptionalRegion()
  {
    final TestCarContext context = TestCarContext.createCarContext(RuntimeEnvironment.getApplication());
    final OrganicMaps sdk = mock(OrganicMaps.class);
    final LocationHelper helper = mock(LocationHelper.class);
    when(sdk.getLocationHelper()).thenReturn(helper);
    final Location location = new Location("gps");
    when(helper.getSavedLocation()).thenReturn(location);
    try (MockedStatic<CountryItem> countries = mockStatic(CountryItem.class))
    {
      countries.when(() -> CountryItem.fill(any())).thenAnswer(call -> new CountryItem(call.getArgument(0)));
      final DownloadMapsScreen screen = new DownloadMapsScreenBuilder(context, sdk)
                                            .setDownloaderType(DownloadMapsScreenBuilder.DownloaderType.FirstLaunch)
                                            .build();
      location.setLatitude(1);
      screen.onResume(screen);
      location.setLatitude(2);
      screen.onResume(screen);
      assertEquals(List.of(new CountryItem("World"), new CountryItem("WorldCoasts"), new CountryItem("Second")),
                   screen.getMissingMaps());
    }
  }
  @Test
  public void onlyUnbundledBuildsOfferMissingWorldMaps()
  {
    try (MockedStatic<CountryItem> countries = mockStatic(CountryItem.class))
    {
      countries.when(() -> CountryItem.fill(any())).thenAnswer(call -> new CountryItem(call.getArgument(0)));
      assertTrue(DownloaderHelpers.isWorldMapsDownloadNeeded("web"));
      assertTrue(DownloaderHelpers.isWorldMapsDownloadNeeded("fdroid"));
      assertFalse(DownloaderHelpers.isWorldMapsDownloadNeeded("google"));
      assertFalse(DownloaderHelpers.isWorldMapsDownloadNeeded("huawei"));
    }
  }

  @Implements(MapManager.class)
  public static class Storage
  {
    @Implementation
    protected static String nativeFindCountry(double lat, double lon)
    {
      return lat == 1 ? "First" : "Second";
    }
  }

  @Implements(OrganicMaps.class)
  public static class Sdk
  {
    @Implementation
    protected static void __staticInitializer__()
    {}
  }
}
