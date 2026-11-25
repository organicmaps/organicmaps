package app.organicmaps.downloader;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

import android.app.Activity;
import android.app.Application;
import android.content.Context;
import android.content.ContextWrapper;
import android.content.res.Resources;
import android.view.ContextThemeWrapper;
import android.view.View;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.ImageView;
import androidx.core.view.ViewCompat;
import app.organicmaps.R;
import app.organicmaps.sdk.downloader.CountryItem;
import app.organicmaps.util.ThemeUtils;
import app.organicmaps.widget.WheelProgressView;
import java.text.NumberFormat;
import java.util.Locale;
import java.util.concurrent.atomic.AtomicInteger;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.MockedStatic;
import org.mockito.Mockito;
import org.robolectric.Robolectric;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.annotation.Config;

@RunWith(RobolectricTestRunner.class)
@Config(sdk = {23, 35}, application = Application.class)
public class DownloaderStatusIconAccessibilityTest
{
  private DownloaderStatusIcon mStatus;
  private ImageView mIcon;
  private WheelProgressView mProgress;
  private CountryItem mCountry;
  private MockedStatic<ThemeUtils> mTheme;
  private Locale mPreviousLocale;
  private final AtomicInteger mDownloads = new AtomicInteger();
  private final AtomicInteger mCancellations = new AtomicInteger();

  @Before
  public void setUp()
  {
    mPreviousLocale = Locale.getDefault();
    Locale.setDefault(Locale.US);
    Activity activity = Robolectric.buildActivity(Activity.class).setup().get();
    Context themed = new ContextThemeWrapper(activity, R.style.MwmTheme);
    Resources original = themed.getResources();
    Resources resources =
        new Resources(original.getAssets(), original.getDisplayMetrics(), original.getConfiguration()) {
          @Override
          public String getString(int id)
          {
            if (id == R.string.downloader_download_map)
              return "Download map";
            if (id == R.string.downloader_retry)
              return "Retry";
            if (id == R.string.downloader_update_map)
              return "Update map";
            if (id == R.string.downloader_map_actions)
              return "Map actions";
            if (id == R.string.downloader_open_group)
              return "Open map list";
            if (id == R.string.cancel_download)
              return "Cancel download";
            if (id == R.string.downloader_queued)
              return "Queued";
            return super.getString(id);
          }
          @Override
          public String getString(int id, Object... args)
          {
            if (id == R.string.downloader_map_action)
              return String.format("%1$s: %2$s", args);
            if (id == R.string.downloader_progress_percent)
              return String.format("Downloading: %s", args);
            if (id == R.string.downloader_applying_percent)
              return String.format("Applying changes: %s", args);
            return super.getString(id, args);
          }
        };
    Context context = new ContextWrapper(themed) {
      @Override
      public Resources getResources()
      {
        return resources;
      }
    };
    FrameLayout root = new FrameLayout(context);
    mIcon = new ImageView(context);
    mIcon.setId(R.id.downloader_status);
    root.addView(mIcon);
    mProgress = new WheelProgressView(context);
    mProgress.setId(R.id.downloader_progress_wheel);
    root.addView(mProgress);
    activity.setContentView(root);
    mTheme = Mockito.mockStatic(ThemeUtils.class, Mockito.CALLS_REAL_METHODS);
    mTheme.when(() -> ThemeUtils.getResource(Mockito.any(), Mockito.anyInt(), Mockito.anyInt()))
        .thenReturn(android.R.drawable.ic_menu_mapmode);
    DownloaderStatusIcon.clearCache();
    mStatus = new DownloaderStatusIcon(root)
                  .setOnIconClickListener(v -> mDownloads.incrementAndGet())
                  .setOnCancelClickListener(v -> mCancellations.incrementAndGet());
    mCountry = new CountryItem("test");
    mCountry.name = "Test map";
  }

  @After
  public void tearDown()
  {
    if (mTheme != null)
      mTheme.close();
    Locale.setDefault(mPreviousLocale);
    DownloaderStatusIcon.clearCache();
  }

  private void update(int status)
  {
    mCountry.status = status;
    mStatus.update(mCountry);
  }

  @Test
  public void iconLabelsFollowTheActionAndMap()
  {
    int[] states = {CountryItem.STATUS_DOWNLOADABLE, CountryItem.STATUS_PARTLY, CountryItem.STATUS_FAILED,
                    CountryItem.STATUS_UPDATABLE, CountryItem.STATUS_DONE};
    String[] actions = {"Download map", "Download map", "Retry", "Update map", "Map actions"};
    for (int i = 0; i < states.length; ++i)
    {
      update(states[i]);
      assertEquals(actions[i] + ": Test map", mIcon.getContentDescription());
      assertEquals(View.VISIBLE, mIcon.getVisibility());
      assertEquals(Button.class.getName(), mIcon.createAccessibilityNodeInfo().getClassName());
    }
    mCountry.name = "Another map";
    update(CountryItem.STATUS_FAILED);
    assertEquals("Retry: Another map", mIcon.getContentDescription());
  }

  @Test
  public void downloadingExposesCancellationAndPercentage()
  {
    mCountry.progress = 34.6f;
    update(CountryItem.STATUS_PROGRESS);
    assertEquals(View.GONE, mIcon.getVisibility());
    assertEquals(View.VISIBLE, mProgress.getVisibility());
    assertEquals("Cancel download: Test map", mProgress.getContentDescription());
    assertEquals("Downloading: 35%", ViewCompat.getStateDescription(mProgress));
    assertEquals(Button.class.getName(), mProgress.createAccessibilityNodeInfo().getClassName());
    assertTrue(mProgress.isFocusable());
    mProgress.performClick();
    assertEquals(1, mCancellations.get());
    assertEquals(0, mDownloads.get());
  }

  @Test
  public void queuedStateDoesNotRetainAnOldPercentage()
  {
    mCountry.progress = 70;
    update(CountryItem.STATUS_PROGRESS);
    update(CountryItem.STATUS_ENQUEUED);
    assertEquals("Queued", ViewCompat.getStateDescription(mProgress));
  }

  @Test
  public void applyingAndCompletedStatesReplaceDownloadProgress()
  {
    mCountry.progress = 95;
    update(CountryItem.STATUS_APPLYING);
    assertEquals("Applying changes: 95%", ViewCompat.getStateDescription(mProgress));
    update(CountryItem.STATUS_DONE);
    assertEquals(View.GONE, mProgress.getVisibility());
    assertNull(ViewCompat.getStateDescription(mProgress));
    assertEquals("Map actions: Test map", mIcon.getContentDescription());
  }

  @Test
  public void percentageUsesTheCurrentLocale()
  {
    Locale.setDefault(Locale.GERMANY);
    mCountry.progress = 42;
    update(CountryItem.STATUS_PROGRESS);
    assertEquals("Downloading: " + NumberFormat.getPercentInstance().format(0.42),
                 ViewCompat.getStateDescription(mProgress));
  }

  @Test
  public void callbacksAndKeyboardFocusAreRemovedWhenDetached()
  {
    update(CountryItem.STATUS_DOWNLOADABLE);
    mIcon.performClick();
    assertEquals(1, mDownloads.get());
    mStatus.setOnIconClickListener(null).setOnCancelClickListener(null);
    assertFalse(mIcon.isClickable());
    assertFalse(mIcon.isFocusable());
    assertFalse(mProgress.isClickable());
    assertFalse(mProgress.isFocusable());
    mProgress.performClick();
    assertEquals(0, mCancellations.get());
  }
}
