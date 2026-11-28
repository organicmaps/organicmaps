package app.organicmaps.downloader;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.Mockito.CALLS_REAL_METHODS;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.spy;

import android.app.Activity;
import android.app.Application;
import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.accessibility.AccessibilityManager;
import android.widget.Button;
import android.widget.TextView;
import androidx.core.view.ViewCompat;
import app.organicmaps.MwmActivity;
import app.organicmaps.R;
import app.organicmaps.sdk.downloader.CountryItem;
import app.organicmaps.sdk.downloader.MapManager;
import app.organicmaps.sdk.routing.RoutingController;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.text.NumberFormat;
import java.util.Locale;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.Robolectric;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.Shadows;
import org.robolectric.annotation.Config;

@RunWith(RobolectricTestRunner.class)
@Config(application = Application.class, sdk = {23, 35}, qualifiers = "en-rUS")
public class DownloadEntryAccessibilityTest
{
  private Activity mActivity;
  private Locale mPreviousLocale;

  @Before
  public void setUp()
  {
    mPreviousLocale = Locale.getDefault();
    Locale.setDefault(Locale.US);
    mActivity = Robolectric.buildActivity(Activity.class).setup().get();
    mActivity.setTheme(R.style.MwmTheme);
    Shadows.shadowOf((AccessibilityManager) mActivity.getSystemService(Context.ACCESSIBILITY_SERVICE)).setEnabled(true);
  }

  @After
  public void tearDown()
  {
    Locale.setDefault(mPreviousLocale);
    OnmapDownloader.setAutodownloadLocked(false);
  }

  private static CountryItem country()
  {
    CountryItem country = new CountryItem("Test_Map");
    country.name = "Nice";
    country.topmostParentId = "Root";
    country.downloadSize = 1024;
    country.progress = 37;
    return country;
  }

  private static void setField(Object target, String name, Object value) throws Exception
  {
    Field field = target.getClass().getDeclaredField(name);
    field.setAccessible(true);
    field.set(target, value);
  }

  private static void call(CountrySuggestFragment fragment, String name, Class<?>[] types, Object... args)
      throws Exception
  {
    Method method = CountrySuggestFragment.class.getDeclaredMethod(name, types);
    method.setAccessible(true);
    method.invoke(fragment, args);
  }

  private void assertProgress(View wheel, String name, int status)
  {
    assertEquals("android.widget.Button", wheel.createAccessibilityNodeInfo().getClassName());
    assertEquals(
        mActivity.getString(R.string.downloader_map_action, mActivity.getString(R.string.cancel_download), name),
        wheel.getContentDescription());
    String percent = NumberFormat.getPercentInstance(mActivity.getResources().getConfiguration().locale).format(.37);
    String expected =
        status == CountryItem.STATUS_ENQUEUED
            ? mActivity.getString(R.string.downloader_queued)
            : mActivity.getString(status == CountryItem.STATUS_APPLYING ? R.string.downloader_applying_percent
                                                                        : R.string.downloader_progress_percent,
                                  percent);
    assertEquals(expected, ViewCompat.getStateDescription(wheel));
  }

  @Test
  public void firstLaunchNamesCancellationAndProgressWithoutChangingItsHandler() throws Exception
  {
    CountrySuggestFragment fragment = spy(new CountrySuggestFragment());
    doReturn(mActivity).when(fragment).requireContext();
    doReturn(true).when(fragment).isAdded();
    View root = LayoutInflater.from(mActivity).inflate(R.layout.fragment_suggest_country_download, null);
    mActivity.setContentView(root);
    call(fragment, "initViews", new Class<?>[] {View.class}, root);
    CountryItem country = country();
    setField(fragment, "mDownloadingCountry", country);
    setField(fragment, "mCurrentCountry", country);
    try (var maps = mockStatic(MapManager.class))
    {
      for (int status :
           new int[] {CountryItem.STATUS_ENQUEUED, CountryItem.STATUS_PROGRESS, CountryItem.STATUS_APPLYING})
      {
        country.status = status;
        call(fragment, "updateProgress", new Class<?>[0]);
        assertProgress(root.findViewById(R.id.wpv__download_progress), country.name, status);
      }
      assertTrue(root.findViewById(R.id.wpv__download_progress).performClick());
      maps.verify(() -> MapManager.cancel(country.id));
      call(fragment, "refreshDownloadButton", new Class<?>[0]);
      Button download = root.findViewById(R.id.btn__download_map);
      assertEquals(mActivity.getString(R.string.downloader_map_action, download.getText(), country.name),
                   download.getContentDescription());
    }
  }

  @Test
  public void onMapNamesQueueDownloadApplyingAndRetryControls() throws Exception
  {
    MwmActivity context = mock(MwmActivity.class, CALLS_REAL_METHODS);
    doReturn(mActivity.getResources()).when(context).getResources();
    View root = LayoutInflater.from(mActivity).inflate(R.layout.onmap_downloader, null);
    mActivity.setContentView(root);
    CountryItem country = country();
    try (var maps = mockStatic(MapManager.class); var routing = mockStatic(RoutingController.class);
         var countries = mockStatic(CountryItem.class))
    {
      routing.when(RoutingController::get).thenReturn(mock(RoutingController.class));
      countries.when(() -> CountryItem.isRoot(anyString())).thenReturn(true);
      OnmapDownloader downloader = new OnmapDownloader(context, root);
      setField(downloader, "mCurrentCountry", country);
      View wheel = root.findViewById(R.id.wheel_downloader_progress);
      for (int status :
           new int[] {CountryItem.STATUS_ENQUEUED, CountryItem.STATUS_PROGRESS, CountryItem.STATUS_APPLYING})
      {
        country.status = status;
        downloader.updateState(false);
        assertProgress(wheel, country.name, status);
      }
      assertTrue(wheel.performClick());
      maps.verify(() -> MapManager.cancel(country.id));
      for (int status : new int[] {CountryItem.STATUS_DOWNLOADABLE, CountryItem.STATUS_FAILED})
      {
        country.status = status;
        downloader.updateState(false);
        TextView button = root.findViewById(R.id.downloader_button);
        assertEquals(mActivity.getString(
                         R.string.downloader_map_action,
                         mActivity.getString(status == CountryItem.STATUS_FAILED ? R.string.downloader_retry
                                                                                 : R.string.downloader_download_map),
                         country.name),
                     button.getContentDescription());
        assertNull(ViewCompat.getStateDescription(wheel));
      }
    }
  }

  @Test
  @Config(qualifiers = "ru")
  public void progressUsesTheInterfaceLocaleRatherThanTheProcessLocale()
  {
    View wheel = new View(mActivity);
    DownloadAccessibility.updateProgress(wheel, "Ницца", CountryItem.STATUS_PROGRESS, 37);
    String percent = NumberFormat.getPercentInstance(new Locale("ru")).format(.37);
    assertEquals(mActivity.getString(R.string.downloader_progress_percent, percent),
                 ViewCompat.getStateDescription(wheel));
  }
}
