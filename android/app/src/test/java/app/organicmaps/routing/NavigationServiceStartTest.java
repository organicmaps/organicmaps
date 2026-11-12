package app.organicmaps.routing;

import static org.junit.Assert.assertThrows;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.CALLS_REAL_METHODS;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;
import static org.robolectric.Shadows.shadowOf;

import android.app.Application;
import android.app.ForegroundServiceStartNotAllowedException;
import android.app.Notification;
import android.content.Context;
import android.content.Intent;
import android.os.Looper;
import androidx.core.app.NotificationCompat;
import androidx.core.app.ServiceCompat;
import androidx.core.content.ContextCompat;
import app.organicmaps.sdk.OrganicMaps;
import app.organicmaps.sdk.location.LocationHelper;
import app.organicmaps.sdk.location.LocationUtils;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.util.log.Logger;
import java.lang.reflect.Field;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.MockedStatic;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;
import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;
import org.robolectric.annotation.LooperMode;

@RunWith(RobolectricTestRunner.class)
@Config(sdk = 28, manifest = Config.NONE, application = Application.class,
        shadows = NavigationServiceStartTest.ShadowOrganicMaps.class)
@LooperMode(LooperMode.Mode.PAUSED)
public class NavigationServiceStartTest
{
  @Implements(value = OrganicMaps.class, isInAndroidSdk = false)
  public static class ShadowOrganicMaps
  {
    @Implementation
    protected static void __staticInitializer__()
    {
      // The service tests mock the initialized SDK instead of loading JNI.
    }
  }

  private MockedStatic<Logger> mLog;
  private MockedStatic<RoutingController> mControllers;
  private MockedStatic<LocationUtils> mPermissions;
  private MockedStatic<ServiceCompat> mPromotion;
  private MockedStatic<ContextCompat> mStarts;
  private RoutingController mController;
  private NavigationService mService;
  private LocationHelper mLocation;

  @Before
  public void setUp() throws ReflectiveOperationException
  {
    mLog = mockStatic(Logger.class);
    mController = mock(RoutingController.class);
    mControllers = mockStatic(RoutingController.class);
    mControllers.when(RoutingController::get).thenReturn(mController);
    mPermissions = mockStatic(LocationUtils.class);
    mPermissions.when(() -> LocationUtils.checkFineLocationPermission(any())).thenReturn(true);
    mPromotion = mockStatic(ServiceCompat.class);
    mStarts = mockStatic(ContextCompat.class);
    mService = mock(NavigationService.class, CALLS_REAL_METHODS);
    doReturn(true).when(mService).stopSelfResult(anyInt());
    mLocation = mock(LocationHelper.class);
    final OrganicMaps maps = mock(OrganicMaps.class);
    when(maps.arePlatformAndCoreInitialized()).thenReturn(true);
    when(maps.getLocationHelper()).thenReturn(mLocation);
    NavigationService.setOrganicMaps(maps);
    final NotificationCompat.Builder notification = mock(NotificationCompat.Builder.class);
    when(notification.build()).thenReturn(mock(Notification.class));
    setNotificationBuilder(notification);
  }

  @After
  public void tearDown() throws ReflectiveOperationException
  {
    NavigationService.stopService(context());
    setNotificationBuilder(null);
    mStarts.close();
    mPromotion.close();
    mPermissions.close();
    mControllers.close();
    mLog.close();
  }

  @Test
  public void refusedPromotionReportsFailureWithoutSubscribingToLocation()
  {
    refusePromotion(new SecurityException());
    mService.onStartCommand(new Intent(), 0, 1);
    verify(mController, never()).onNavigationStartFailed();
    shadowOf(Looper.getMainLooper()).idle();
    verify(mController).onNavigationStartFailed();
    verify(mLocation, never()).addListener(any());
    verify(mLocation, never()).restartWithNewMode();
  }

  @Test
  public void permissionRevokedBeforeDeliveryAlsoReportsFailure()
  {
    mPermissions.when(() -> LocationUtils.checkFineLocationPermission(any())).thenReturn(false);
    mService.onStartCommand(new Intent(), 0, 1);
    shadowOf(Looper.getMainLooper()).idle();
    verify(mController).onNavigationStartFailed();
    mPromotion.verifyNoInteractions();
  }

  @Test
  @Config(sdk = 34)
  public void backgroundStartRestrictionAlsoReportsFailure()
  {
    refusePromotion(new ForegroundServiceStartNotAllowedException("Background start refused"));
    mService.onStartCommand(new Intent(), 0, 1);
    shadowOf(Looper.getMainLooper()).idle();
    verify(mController).onNavigationStartFailed();
  }

  @Test
  public void supersededServiceCommandDoesNotFailTheNewerStart()
  {
    refusePromotion(new SecurityException());
    doReturn(false).when(mService).stopSelfResult(1);
    mService.onStartCommand(new Intent(), 0, 1);
    shadowOf(Looper.getMainLooper()).idle();
    verify(mController, never()).onNavigationStartFailed();
  }

  @Test
  public void cancellationDiscardsTheQueuedStartFailure()
  {
    refuseStart();
    NavigationService.startForegroundService(context());
    NavigationService.stopService(context());
    shadowOf(Looper.getMainLooper()).idle();
    verify(mController, never()).onNavigationStartFailed();
  }

  @Test
  public void newStartDiscardsThePreviousRequestsFailure()
  {
    refuseStart();
    NavigationService.startForegroundService(context());
    mStarts.reset();
    NavigationService.startForegroundService(context());
    shadowOf(Looper.getMainLooper()).idle();
    verify(mController, never()).onNavigationStartFailed();
  }

  @Test
  public void appOpsRefusalReportsFailureAfterTheCallerReturns()
  {
    refuseStart();
    NavigationService.startForegroundService(context());
    verify(mController, never()).onNavigationStartFailed();
    shadowOf(Looper.getMainLooper()).idle();
    verify(mController).onNavigationStartFailed();
  }

  @Test
  @Config(sdk = 34)
  public void backgroundRequestRefusalReportsFailureAfterTheCallerReturns()
  {
    mStarts.when(() -> ContextCompat.startForegroundService(any(), any()))
        .thenThrow(new ForegroundServiceStartNotAllowedException("Background start refused"));
    NavigationService.startForegroundService(context());
    verify(mController, never()).onNavigationStartFailed();
    shadowOf(Looper.getMainLooper()).idle();
    verify(mController).onNavigationStartFailed();
  }

  @Test
  public void unrelatedRequestErrorsPropagate()
  {
    mStarts.when(() -> ContextCompat.startForegroundService(any(), any()))
        .thenThrow(new IllegalStateException("Unexpected service state"));
    assertThrows(IllegalStateException.class, () -> NavigationService.startForegroundService(context()));
    shadowOf(Looper.getMainLooper()).idle();
    verify(mController, never()).onNavigationStartFailed();
  }

  @Test
  public void unrelatedPromotionErrorsPropagate()
  {
    refusePromotion(new IllegalStateException("Invalid service configuration"));
    assertThrows(IllegalStateException.class, () -> mService.onStartCommand(new Intent(), 0, 1));
    shadowOf(Looper.getMainLooper()).idle();
    verify(mController, never()).onNavigationStartFailed();
  }

  @Test
  public void successfulPromotionStartsGuidance()
  {
    mService.onStartCommand(new Intent(), 0, 1);
    shadowOf(Looper.getMainLooper()).idle();
    verify(mLocation).addListener(mService);
    verify(mLocation).restartWithNewMode();
    verify(mController, never()).onNavigationStartFailed();
  }

  private void refusePromotion(RuntimeException exception)
  {
    mPromotion.when(() -> ServiceCompat.startForeground(eq(mService), anyInt(), any(), anyInt())).thenThrow(exception);
  }

  private void refuseStart()
  {
    mStarts.when(() -> ContextCompat.startForegroundService(any(), any()))
        .thenThrow(new SecurityException("Foreground not allowed as per app op"));
  }

  private static Context context()
  {
    return RuntimeEnvironment.getApplication();
  }

  private static void setNotificationBuilder(NotificationCompat.Builder builder) throws ReflectiveOperationException
  {
    final Field field = NavigationService.class.getDeclaredField("mNotificationBuilder");
    field.setAccessible(true);
    field.set(null, builder);
  }
}
