package app.organicmaps;

import static android.Manifest.permission.ACCESS_FINE_LOCATION;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;

import android.app.Application;
import android.os.Looper;
import androidx.activity.result.ActivityResultLauncher;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.lifecycle.Lifecycle;
import androidx.lifecycle.LifecycleRegistry;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.bookmarks.data.BookmarkManager;
import app.organicmaps.sdk.location.TrackRecorder;
import app.organicmaps.sdk.util.log.Logger;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.MockedStatic;
import org.robolectric.Robolectric;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.Shadows;
import org.robolectric.annotation.Config;
import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;
import org.robolectric.util.ReflectionHelpers;

@RunWith(RobolectricTestRunner.class)
@Config(application = Application.class, sdk = 28,
        shadows = {MwmActivityRecordingRestoreTest.BookmarkLoadingShadow.class,
                   MwmActivityRecordingRestoreTest.RecordingNativeShadow.class,
                   MwmActivityRecordingRestoreTest.FrameworkShadow.class})
public class MwmActivityRecordingRestoreTest
{
  @Implements(value = BookmarkManager.class, isInAndroidSdk = false)
  public static class BookmarkLoadingShadow
  {
    static int sCompletedLoads;
    static boolean sLoading;

    @Implementation
    protected int getLoadingGeneration()
    {
      return sCompletedLoads;
    }

    @Implementation
    protected boolean isAsyncBookmarksLoadingInProgress()
    {
      return sLoading;
    }
  }

  @Implements(value = TrackRecorder.class, isInAndroidSdk = false)
  public static class RecordingNativeShadow
  {
    static int sSaves;
    static boolean sShowTrack;

    @Implementation
    protected static boolean nativeIsTrackRecordingEnabled()
    {
      return true;
    }

    @Implementation
    protected static int nativeSaveTrackRecordingWithName(String name, boolean showTrack)
    {
      ++sSaves;
      sShowTrack = showTrack;
      return TrackRecorder.SAVE_FILE_ERROR;
    }
  }

  @Implements(value = Framework.class, isInAndroidSdk = false)
  public static class FrameworkShadow
  {
    @Implementation
    protected static String[] nativeGetBookmarksFilesExts()
    {
      return new String[] {".kml"};
    }
  }

  public static class RecordingActivity extends MwmActivity
  {
    @Nullable
    private Lifecycle mTestLifecycle;

    @NonNull
    @Override
    public Lifecycle getLifecycle()
    {
      return mTestLifecycle == null ? super.getLifecycle() : mTestLifecycle;
    }
  }

  private MockedStatic<Logger> mLogger;
  private RecordingActivity mActivity;
  private LifecycleRegistry mLifecycle;

  @Before
  public void setUp()
  {
    mLogger = mockStatic(Logger.class);
    RecordingNativeShadow.sSaves = 0;
    RecordingNativeShadow.sShowTrack = true;
    BookmarkLoadingShadow.sCompletedLoads = 0;
    BookmarkLoadingShadow.sLoading = false;
    Shadows.shadowOf(RuntimeEnvironment.getApplication()).denyPermissions(ACCESS_FINE_LOCATION);

    mActivity = Robolectric.buildActivity(RecordingActivity.class).get();
    mActivity.setTheme(R.style.MwmTheme);
    mLifecycle = new LifecycleRegistry(mActivity);
    mActivity.mTestLifecycle = mLifecycle;
    mLifecycle.setCurrentState(Lifecycle.State.RESUMED);
    ReflectionHelpers.setField(mActivity, "mLocationPermissionRequest", mock(ActivityResultLauncher.class));
  }

  @After
  public void tearDown()
  {
    mLogger.close();
  }

  @Test
  public void waitsForFirstCompletedLoadEvenBeforeNativeSetsTheBusyFlag()
  {
    mActivity.restoreTrackRecording();
    assertEquals(0, RecordingNativeShadow.sSaves);

    finishLoading();
    Shadows.shadowOf(Looper.getMainLooper()).idle();
    finishLoading();
    Shadows.shadowOf(Looper.getMainLooper()).idle();
    assertEquals(1, RecordingNativeShadow.sSaves);
    assertFalse(RecordingNativeShadow.sShowTrack);
  }

  @Test
  public void waitsForAnotherLoadWhenBookmarksWereLoadedBefore()
  {
    BookmarkLoadingShadow.sCompletedLoads = 1;
    BookmarkLoadingShadow.sLoading = true;

    mActivity.restoreTrackRecording();
    assertEquals(0, RecordingNativeShadow.sSaves);

    finishLoading();
    Shadows.shadowOf(Looper.getMainLooper()).idle();
    assertEquals(1, RecordingNativeShadow.sSaves);
    assertFalse(RecordingNativeShadow.sShowTrack);
  }

  @Test
  public void resumesPendingSaveAfterLoadCompletedWhileStopped()
  {
    mActivity.restoreTrackRecording();
    mLifecycle.setCurrentState(Lifecycle.State.CREATED);
    // OnStop removed the Activity's listener, so it misses this completion.
    BookmarkLoadingShadow.sCompletedLoads = 1;

    mLifecycle.setCurrentState(Lifecycle.State.STARTED);
    mActivity.scheduleRestoredRecordingSave();
    assertEquals(0, RecordingNativeShadow.sSaves);
    mLifecycle.setCurrentState(Lifecycle.State.RESUMED);
    Shadows.shadowOf(Looper.getMainLooper()).idle();

    assertEquals(1, RecordingNativeShadow.sSaves);
    assertFalse(RecordingNativeShadow.sShowTrack);
  }

  @Test
  public void permissionRestoredBeforeTheDeferredSaveKeepsTheRecording()
  {
    mActivity.restoreTrackRecording();
    finishLoading();
    Shadows.shadowOf(RuntimeEnvironment.getApplication()).grantPermissions(ACCESS_FINE_LOCATION);
    Shadows.shadowOf(Looper.getMainLooper()).idle();

    assertEquals(0, RecordingNativeShadow.sSaves);
  }

  @Test
  public void aFinishingActivityCannotConsumeTheDeferredSave()
  {
    mActivity.restoreTrackRecording();
    finishLoading();
    mActivity.finish();
    Shadows.shadowOf(Looper.getMainLooper()).idle();

    assertEquals(0, RecordingNativeShadow.sSaves);
  }

  private void finishLoading()
  {
    ++BookmarkLoadingShadow.sCompletedLoads;
    BookmarkLoadingShadow.sLoading = false;
    mActivity.onBookmarksLoadingFinished();
  }
}
