package app.organicmaps.sdk.location.gms;

import static org.junit.Assert.assertEquals;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.clearInvocations;
import static org.mockito.Mockito.inOrder;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.app.PendingIntent;
import android.content.Context;
import android.location.Location;
import android.os.Looper;
import app.organicmaps.sdk.location.BaseLocationProvider;
import app.organicmaps.sdk.location.LocationUtils;
import app.organicmaps.sdk.util.log.Logger;
import com.google.android.gms.common.api.ResolvableApiException;
import com.google.android.gms.location.FusedLocationProviderClient;
import com.google.android.gms.location.LocationAvailability;
import com.google.android.gms.location.LocationCallback;
import com.google.android.gms.location.LocationRequest;
import com.google.android.gms.location.LocationResult;
import com.google.android.gms.location.LocationSettingsRequest;
import com.google.android.gms.location.LocationSettingsResponse;
import com.google.android.gms.location.LocationSettingsStatusCodes;
import com.google.android.gms.location.SettingsClient;
import com.google.android.gms.tasks.CancellationToken;
import com.google.android.gms.tasks.OnTokenCanceledListener;
import com.google.android.gms.tasks.TaskCompletionSource;
import com.google.android.gms.tasks.Tasks;
import java.util.ArrayList;
import java.util.List;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.InOrder;
import org.mockito.MockedStatic;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.Shadows;
import org.robolectric.annotation.Config;

@RunWith(RobolectricTestRunner.class)
@Config(manifest = Config.NONE, sdk = 28)
public class GoogleFusedLocationProviderTest
{
  // microG's CancellationTokenSource does not notify token listeners.
  private static final class Cancellation extends CancellationToken
  {
    private final List<OnTokenCanceledListener> mListeners = new ArrayList<>();
    private boolean mCanceled;

    @Override
    public boolean isCancellationRequested()
    {
      return mCanceled;
    }

    @Override
    public CancellationToken onCanceledRequested(OnTokenCanceledListener listener)
    {
      mListeners.add(listener);
      return this;
    }

    void cancel()
    {
      mCanceled = true;
      for (OnTokenCanceledListener listener : mListeners)
        listener.onCanceled();
    }
  }

  private final Context mContext = mock(Context.class);
  private final BaseLocationProvider.Listener mListener = mock(BaseLocationProvider.Listener.class);
  private final FusedLocationProviderClient mLocationClient = mock(FusedLocationProviderClient.class);
  private final SettingsClient mSettingsClient = mock(SettingsClient.class);
  private final List<TaskCompletionSource<LocationSettingsResponse>> mSettings = new ArrayList<>();
  private final List<TaskCompletionSource<Void>> mRegistrations = new ArrayList<>();
  private final List<Cancellation> mCancellations = new ArrayList<>();
  private final List<LocationCallback> mCallbacks = new ArrayList<>();
  private GoogleFusedLocationProvider mProvider;
  private MockedStatic<Logger> mLogger;
  private MockedStatic<LocationUtils> mLocationUtils;

  @Before
  public void setUp()
  {
    mLogger = mockStatic(Logger.class);
    mLocationUtils = mockStatic(LocationUtils.class);
    mLocationUtils.when(() -> LocationUtils.areLocationServicesTurnedOn(mContext)).thenReturn(true);
    when(mSettingsClient.checkLocationSettings(any(LocationSettingsRequest.class))).thenAnswer(invocation -> {
      final TaskCompletionSource<LocationSettingsResponse> task = new TaskCompletionSource<>();
      mSettings.add(task);
      return task.getTask();
    });
    when(mLocationClient.requestLocationUpdates(any(LocationRequest.class), any(LocationCallback.class),
                                                any(Looper.class)))
        .thenAnswer(invocation -> {
          final Cancellation cancellation = new Cancellation();
          final TaskCompletionSource<Void> task = new TaskCompletionSource<>(cancellation);
          mCancellations.add(cancellation);
          mRegistrations.add(task);
          mCallbacks.add(invocation.getArgument(1));
          return task.getTask();
        });
    when(mLocationClient.removeLocationUpdates(any(LocationCallback.class))).thenReturn(Tasks.forResult(null));
    mProvider = new GoogleFusedLocationProvider(mContext, mListener, mLocationClient, mSettingsClient);
  }

  @After
  public void tearDown()
  {
    mLocationUtils.close();
    mLogger.close();
  }

  private void register(int session)
  {
    mSettings.get(session).setResult(null);
    Shadows.shadowOf(Looper.getMainLooper()).idle();
  }

  private LocationResult result(Location location)
  {
    final LocationResult result = mock(LocationResult.class);
    when(result.getLastLocation()).thenReturn(location);
    return result;
  }

  private void verifyNoLocationEvents()
  {
    verify(mListener, never()).onLocationChanged(any(Location.class));
    verify(mListener, never()).onLocationDisabled();
    verify(mListener, never()).onFusedLocationFailure();
    verify(mListener, never()).onLocationResolutionRequired(any(PendingIntent.class));
  }

  @Test
  public void startupProgressTracksTaskCompletion()
  {
    final InOrder order = inOrder(mListener);
    mProvider.start(100);
    order.verify(mListener).onFusedLocationProgress(BaseLocationProvider.StartupPhase.CHECKING_SETTINGS);
    register(0);
    order.verify(mListener).onFusedLocationProgress(BaseLocationProvider.StartupPhase.REGISTERING_UPDATES);
    mRegistrations.get(0).setResult(null);
    Shadows.shadowOf(Looper.getMainLooper()).idle();
    order.verify(mListener).onFusedLocationProgress(BaseLocationProvider.StartupPhase.REGISTERED);
    order.verifyNoMoreInteractions();
  }

  @Test
  public void fixCanArriveBeforeRegistrationCompletion()
  {
    mProvider.start(100);
    register(0);
    final Location location = mock(Location.class);
    mCallbacks.get(0).onLocationResult(result(location));
    final InOrder order = inOrder(mListener);
    order.verify(mListener).onFusedLocationProgress(BaseLocationProvider.StartupPhase.CHECKING_SETTINGS);
    order.verify(mListener).onFusedLocationProgress(BaseLocationProvider.StartupPhase.REGISTERING_UPDATES);
    order.verify(mListener).onLocationChanged(location);
    mRegistrations.get(0).setResult(null);
    Shadows.shadowOf(Looper.getMainLooper()).idle();
    order.verify(mListener).onFusedLocationProgress(BaseLocationProvider.StartupPhase.REGISTERED);
  }

  @Test
  public void unavailableHintDoesNotFailARegisteredRequest()
  {
    mProvider.start(100);
    register(0);
    mRegistrations.get(0).setResult(null);
    Shadows.shadowOf(Looper.getMainLooper()).idle();
    final LocationAvailability availability = mock(LocationAvailability.class);
    when(availability.isLocationAvailable()).thenReturn(false);
    mCallbacks.get(0).onLocationAvailability(availability);
    verifyNoLocationEvents();
    final Location location = mock(Location.class);
    mCallbacks.get(0).onLocationResult(result(location));
    verify(mListener).onLocationChanged(location);
  }

  @Test
  public void stoppedSettingsRequestCannotRegisterUpdates()
  {
    mProvider.start(100);
    mProvider.stop();
    register(0);

    assertEquals(0, mCallbacks.size());
    verifyNoLocationEvents();
  }

  @Test
  public void olderSettingsCompletionCannotReplaceTheNewRequest()
  {
    mProvider.start(100);
    mProvider.stop();
    mProvider.start(3000);
    register(0);
    register(1);

    assertEquals(1, mCallbacks.size());
    final Location location = mock(Location.class);
    mCallbacks.get(0).onLocationResult(result(location));
    verify(mListener).onLocationChanged(location);
  }

  @Test
  public void lateRegistrationSuccessRemovesOnlyItsOwnCallback()
  {
    mProvider.start(100);
    register(0);
    final LocationCallback oldCallback = mCallbacks.get(0);
    mProvider.stop();
    clearInvocations(mLocationClient);
    mProvider.start(3000);
    register(1);
    final LocationCallback currentCallback = mCallbacks.get(1);
    mRegistrations.get(1).setResult(null);
    mRegistrations.get(0).setResult(null);
    Shadows.shadowOf(Looper.getMainLooper()).idle();

    verify(mListener).onFusedLocationProgress(BaseLocationProvider.StartupPhase.REGISTERED);
    verify(mLocationClient).removeLocationUpdates(oldCallback);
    verify(mLocationClient, never()).removeLocationUpdates(currentCallback);
    final Location oldLocation = mock(Location.class);
    oldCallback.onLocationResult(result(oldLocation));
    verifyNoLocationEvents();
    final Location currentLocation = mock(Location.class);
    currentCallback.onLocationResult(result(currentLocation));
    verify(mListener).onLocationChanged(currentLocation);
  }

  @Test
  public void registrationFailureFallsBackAndInvalidatesDelivery()
  {
    mProvider.start(100);
    register(0);
    mRegistrations.get(0).setException(new IllegalStateException("Unavailable API"));
    Shadows.shadowOf(Looper.getMainLooper()).idle();

    verify(mListener).onFusedLocationFailure();
    verify(mListener, never()).onLocationDisabled();
    mCallbacks.get(0).onLocationResult(result(mock(Location.class)));
    verify(mListener, never()).onLocationChanged(any(Location.class));
  }

  @Test
  public void registrationCancellationFallsBack()
  {
    mProvider.start(100);
    register(0);
    mCancellations.get(0).cancel();
    Shadows.shadowOf(Looper.getMainLooper()).idle();

    verify(mListener).onFusedLocationFailure();
  }

  @Test
  public void settingsCancellationFallsBack()
  {
    final Cancellation cancellation = new Cancellation();
    final TaskCompletionSource<LocationSettingsResponse> task = new TaskCompletionSource<>(cancellation);
    when(mSettingsClient.checkLocationSettings(any(LocationSettingsRequest.class))).thenReturn(task.getTask());
    mProvider.start(100);
    cancellation.cancel();
    Shadows.shadowOf(Looper.getMainLooper()).idle();

    verify(mListener).onFusedLocationFailure();
    assertEquals(0, mCallbacks.size());
  }

  @Test
  public void optionalGoogleSettingsDoNotPreventSatelliteLocation()
  {
    mProvider.start(100);
    final ResolvableApiException error = mock(ResolvableApiException.class);
    when(error.getStatusCode()).thenReturn(LocationSettingsStatusCodes.RESOLUTION_REQUIRED);
    mSettings.get(0).setException(error);
    Shadows.shadowOf(Looper.getMainLooper()).idle();

    assertEquals(1, mCallbacks.size());
    verifyNoLocationEvents();
  }

  @Test
  public void resolutionIsRequestedOnlyForTheCurrentSession()
  {
    final PendingIntent intent = mock(PendingIntent.class);
    final ResolvableApiException error = mock(ResolvableApiException.class);
    when(error.getStatusCode()).thenReturn(LocationSettingsStatusCodes.RESOLUTION_REQUIRED);
    when(error.getResolution()).thenReturn(intent);
    mLocationUtils.when(() -> LocationUtils.areLocationServicesTurnedOn(mContext)).thenReturn(false);
    mProvider.start(100);
    mProvider.stop();
    mProvider.start(3000);
    mSettings.get(0).setException(error);
    Shadows.shadowOf(Looper.getMainLooper()).idle();
    verifyNoLocationEvents();
    mSettings.get(1).setException(error);
    Shadows.shadowOf(Looper.getMainLooper()).idle();

    verify(mListener).onLocationResolutionRequired(intent);
    assertEquals(0, mCallbacks.size());
  }

  @Test
  public void synchronousRegistrationFailureUsesTheSameRecoveryPath()
  {
    when(mLocationClient.requestLocationUpdates(any(LocationRequest.class), any(LocationCallback.class),
                                                any(Looper.class)))
        .thenThrow(new SecurityException("Permission revoked"));
    mProvider.start(100);
    register(0);

    verify(mListener).onFusedLocationFailure();
  }
}
