package app.organicmaps.downloader;

import static org.junit.Assert.assertEquals;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.text.TextUtils;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;
import androidx.core.view.ViewCompat;
import app.organicmaps.MwmActivity;
import app.organicmaps.R;
import app.organicmaps.sdk.downloader.CountryItem;
import app.organicmaps.sdk.downloader.MapManager;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.util.UiUtils;
import app.organicmaps.widget.WheelProgressView;
import java.lang.reflect.Field;
import java.util.List;
import org.junit.Test;

public class OnmapDownloaderTest
{
  @Test
  public void viewportAndStatusRefreshKeepTheSameMapOnlyProgressAsCallbacks() throws Exception
  {
    MwmActivity activity = mock(MwmActivity.class);
    View frame = mock(View.class);
    View controls = mock(View.class);
    WheelProgressView wheel = mock(WheelProgressView.class);
    when(frame.findViewById(R.id.downloader_parent)).thenReturn(mock(TextView.class));
    when(frame.findViewById(R.id.downloader_title)).thenReturn(mock(TextView.class));
    when(frame.findViewById(R.id.downloader_size)).thenReturn(mock(TextView.class));
    when(frame.findViewById(R.id.downloader_controls_frame)).thenReturn(controls);
    when(controls.findViewById(R.id.wheel_downloader_progress)).thenReturn(wheel);
    when(controls.findViewById(R.id.downloader_button)).thenReturn(mock(Button.class));

    CountryItem country = spy(new CountryItem("France_Nice"));
    country.name = "Nice";
    country.topmostParentId = "France";
    country.status = CountryItem.STATUS_PROGRESS;
    country.updateProgress(new MapManager.Progress(50, 100, 0, 100));
    doAnswer(invocation -> {
      country.updateProgress(new MapManager.Progress(50, 100, 0, 100));
      return null;
    })
        .when(country)
        .update();

    try (var maps = mockStatic(MapManager.class); var countries = mockStatic(CountryItem.class);
         var routing = mockStatic(RoutingController.class); var text = mockStatic(TextUtils.class);
         var insets = mockStatic(ViewCompat.class); var visibility = mockStatic(UiUtils.class))
    {
      countries.when(() -> CountryItem.fill(country.id)).thenReturn(country);
      countries.when(() -> CountryItem.isRoot(country.topmostParentId)).thenReturn(true);
      routing.when(RoutingController::get).thenReturn(mock(RoutingController.class));
      maps.when(() -> MapManager.nativeGetOverallProgressBytes(any(String[].class), eq(false)))
          .thenReturn(new long[] {50, 100});

      OnmapDownloader downloader = new OnmapDownloader(activity, frame);
      Field field = OnmapDownloader.class.getDeclaredField("mCountryChangedListener");
      field.setAccessible(true);
      var viewport = (MapManager.CurrentCountryChangedListener) field.get(downloader);
      field = OnmapDownloader.class.getDeclaredField("mStorageCallback");
      field.setAccessible(true);
      var storage = (MapManager.StorageCallback) field.get(downloader);

      viewport.onCurrentCountryChanged(country.id);
      storage.onStatusChanged(List.of(
          new MapManager.StorageCallbackData(country.id, CountryItem.STATUS_PROGRESS, CountryItem.ERROR_NONE, true)));
      storage.onProgress(country.id, new MapManager.Progress(50, 100, 80, 100));

      verify(wheel, times(3)).setProgress(48);
      assertEquals(50, country.downloadedBytes);
      assertEquals(100, country.bytesToDownload);
      country.present = true;
      storage.onStatusChanged(List.of(
          new MapManager.StorageCallbackData(country.id, CountryItem.STATUS_PROGRESS, CountryItem.ERROR_NONE, true)));
      maps.verify(() -> MapManager.nativeGetOverallProgressBytes(any(String[].class), eq(false)), times(2));
    }
  }
}
