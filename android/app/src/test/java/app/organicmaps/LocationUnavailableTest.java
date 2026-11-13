package app.organicmaps;

import static android.Manifest.permission.ACCESS_COARSE_LOCATION;
import static android.Manifest.permission.ACCESS_FINE_LOCATION;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.app.Application;
import android.app.Notification;
import android.app.NotificationManager;
import android.content.Intent;
import android.location.Location;
import android.location.LocationManager;
import android.os.Looper;
import android.provider.Settings;
import android.widget.TextView;
import androidx.activity.result.ActivityResultLauncher;
import androidx.appcompat.app.AlertDialog;
import app.organicmaps.location.TrackRecordingService;
import app.organicmaps.sdk.Map;
import app.organicmaps.sdk.location.LocationHelper;
import app.organicmaps.sdk.location.LocationState;
import app.organicmaps.sdk.util.log.Logger;
import app.organicmaps.util.Utils;
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
import org.robolectric.shadows.ShadowDialog;
import org.robolectric.util.ReflectionHelpers;

@RunWith(RobolectricTestRunner.class)
@Config(application = Application.class, sdk = 28, shadows = LocationUnavailableTest.LocationStateShadow.class)
public class LocationUnavailableTest
{
  @Implements(value = LocationState.class, isInAndroidSdk = false)
  public static class LocationStateShadow
  {
    static int sMode;
    static int sSwitches;

    @Implementation
    protected static int nativeGetMode()
    {
      return sMode;
    }

    @Implementation
    protected static void nativeSwitchToNextMode()
    {
      ++sSwitches;
    }
  }

  private final LocationHelper mLocationHelper = mock(LocationHelper.class);
  @SuppressWarnings("unchecked")
  private final ActivityResultLauncher<String[]> mPermissions = mock(ActivityResultLauncher.class);
  @SuppressWarnings("unchecked")
  private final ActivityResultLauncher<Intent> mSettings = mock(ActivityResultLauncher.class);
  private final Intent mSettingsIntent = new Intent(Settings.ACTION_LOCATION_SOURCE_SETTINGS);
  private MwmActivity mActivity;
  private MockedStatic<Logger> mLogger;
  private MockedStatic<Map> mMap;
  private MockedStatic<MwmApplication> mApplication;
  private MockedStatic<Utils> mUtils;

  @Before
  public void setUp()
  {
    mLogger = mockStatic(Logger.class);
    mMap = mockStatic(Map.class);
    mMap.when(Map::isEngineCreated).thenReturn(true);
    mApplication = mockStatic(MwmApplication.class);
    final MwmApplication application = mock(MwmApplication.class);
    when(application.getLocationHelper()).thenReturn(mLocationHelper);
    mApplication.when(() -> MwmApplication.from(any())).thenReturn(application);
    mUtils = mockStatic(Utils.class);
    mUtils.when(() -> Utils.makeSystemLocationSettingIntent(any())).thenReturn(mSettingsIntent);
    LocationStateShadow.sMode = LocationState.PENDING_POSITION;
    LocationStateShadow.sSwitches = 0;
    Shadows.shadowOf(RuntimeEnvironment.getApplication()).grantPermissions(ACCESS_COARSE_LOCATION);
    Shadows.shadowOf(RuntimeEnvironment.getApplication()).denyPermissions(ACCESS_FINE_LOCATION);
    Shadows.shadowOf(RuntimeEnvironment.getApplication().getSystemService(LocationManager.class))
        .setLocationEnabled(true);

    mActivity = Robolectric.buildActivity(MwmActivity.class).get();
    mActivity.setTheme(R.style.MwmTheme);
    ReflectionHelpers.setField(mActivity, "mLocationPermissionRequest", mPermissions);
    ReflectionHelpers.setField(mActivity, "mLocationSettingsRequest", mSettings);
  }

  @After
  public void tearDown()
  {
    if (ShadowDialog.getLatestDialog() != null)
      ShadowDialog.getLatestDialog().dismiss();
    mUtils.close();
    mApplication.close();
    mMap.close();
    mLogger.close();
  }

  private AlertDialog showUnavailable(boolean precise)
  {
    if (precise)
      Shadows.shadowOf(RuntimeEnvironment.getApplication()).grantPermissions(ACCESS_FINE_LOCATION);
    mActivity.onLocationUnavailable();
    final AlertDialog dialog = (AlertDialog) ShadowDialog.getLatestDialog();
    assertNotNull("No location recovery dialog", dialog);
    return dialog;
  }

  @Test
  public void coarseOnlyFailureOffersPrecisePermissionAndSettings()
  {
    final AlertDialog dialog = showUnavailable(false);
    final TextView message = dialog.findViewById(android.R.id.message);
    assertEquals(mActivity.getString(R.string.precise_location_is_disabled_long_text), message.getText());
    assertEquals(mActivity.getString(R.string.location_settings),
                 dialog.getButton(AlertDialog.BUTTON_NEUTRAL).getText());
    clickButton(dialog, AlertDialog.BUTTON_POSITIVE);
    verify(mPermissions).launch(new String[] {ACCESS_COARSE_LOCATION, ACCESS_FINE_LOCATION});
  }

  @Test
  public void preciseFailureCanRetryPendingAcquisition()
  {
    final AlertDialog dialog = showUnavailable(true);
    clickButton(dialog, AlertDialog.BUTTON_POSITIVE);
    verify(mLocationHelper).resumeLocationInForeground();
    assertFalse(dialog.isShowing());
  }

  @Test
  public void retryReentersPositionModeAfterTerminalFailure()
  {
    LocationStateShadow.sMode = LocationState.NOT_FOLLOW_NO_POSITION;
    clickButton(showUnavailable(true), AlertDialog.BUTTON_POSITIVE);
    assertEquals(1, LocationStateShadow.sSwitches);
  }

  @Test
  public void settingsActionUsesTheResultLauncher()
  {
    clickButton(showUnavailable(true), AlertDialog.BUTTON_NEUTRAL);
    verify(mSettings).launch(mSettingsIntent);
  }

  @Test
  public void recordingRetryDoesNotLeakIntoOrdinaryTimeouts()
  {
    final TrackRecordingService service = Robolectric.buildService(TrackRecordingService.class).get();
    TrackRecordingService.createNotificationChannel(service);
    service.onLocationUnavailable();
    final Notification unavailable = notification(service);
    assertEquals(2, unavailable.actions.length);
    assertEquals(service.getString(R.string.try_again), unavailable.actions[1].title);
    assertTrue(Shadows.shadowOf(unavailable.actions[1].actionIntent).isService());
    final Intent retry = Shadows.shadowOf(unavailable.actions[1].actionIntent).getSavedIntent();
    assertEquals(TrackRecordingService.class.getName(), retry.getComponent().getClassName());
    assertFalse(TrackRecordingService.STOP_TRACK_RECORDING.equals(retry.getAction()));

    service.onLocationUpdateTimeout();
    assertEquals(1, notification(service).actions.length);
    service.onLocationUpdated(new Location("gps"));
    assertEquals(service.getString(R.string.track_recording),
                 notification(service).extras.getCharSequence(Notification.EXTRA_TITLE));
  }

  private static Notification notification(TrackRecordingService service)
  {
    final NotificationManager manager = service.getSystemService(NotificationManager.class);
    assertEquals(1, manager.getActiveNotifications().length);
    assertEquals(TrackRecordingService.TRACK_REC_NOTIFICATION_ID, manager.getActiveNotifications()[0].getId());
    return manager.getActiveNotifications()[0].getNotification();
  }

  private static void clickButton(AlertDialog dialog, int button)
  {
    dialog.getButton(button).performClick();
    Shadows.shadowOf(Looper.getMainLooper()).idle();
  }
}
