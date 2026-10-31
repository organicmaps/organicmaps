package app.organicmaps.sdk.car.renderer;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotSame;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.Mockito.clearInvocations;
import static org.mockito.Mockito.inOrder;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockConstruction;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyNoMoreInteractions;
import static org.mockito.Mockito.when;

import android.graphics.Rect;
import android.hardware.display.VirtualDisplay;
import androidx.car.app.CarContext;
import androidx.car.app.SurfaceContainer;
import androidx.core.content.ContextCompat;
import androidx.lifecycle.LifecycleOwner;
import app.organicmaps.sdk.MapRenderingListener;
import app.organicmaps.sdk.display.DisplayManager;
import app.organicmaps.sdk.location.LocationHelper;
import app.organicmaps.sdk.util.log.Logger;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.mockito.InOrder;
import org.mockito.MockedConstruction;
import org.mockito.MockedStatic;

public class SurfaceCallbackTest
{
  private final SurfaceContainer mSurface = mock(SurfaceContainer.class);
  private SurfaceCallback mCallback;
  private MockedConstruction<Rect> mRects;
  private MockedConstruction<CarPresentation> mPresentations;
  private MockedStatic<ContextCompat> mContextServices;
  private MockedStatic<Logger> mLogger;

  @Before
  public void setUp()
  {
    mRects = mockConstruction(Rect.class);
    mLogger = mockStatic(Logger.class);
    mContextServices = mockStatic(ContextCompat.class);
    mPresentations = mockConstruction(CarPresentation.class);
    final CarContext carContext = mock(CarContext.class);
    final android.hardware.display.DisplayManager displayManager = mock(android.hardware.display.DisplayManager.class);
    final VirtualDisplay virtualDisplay = mock(VirtualDisplay.class);
    mContextServices
        .when(() -> ContextCompat.getSystemService(carContext, android.hardware.display.DisplayManager.class))
        .thenReturn(displayManager);
    when(displayManager.createVirtualDisplay(anyString(), anyInt(), anyInt(), anyInt(), any(), anyInt()))
        .thenReturn(virtualDisplay);
    mCallback = new SurfaceCallback(carContext, mock(LifecycleOwner.class), mock(LocationHelper.class),
                                    mock(MapRenderingListener.class), mock(DisplayManager.class));
  }

  @After
  public void tearDown()
  {
    mCallback.destroy();
    mPresentations.close();
    mContextServices.close();
    mLogger.close();
    mRects.close();
  }

  @Test
  public void updatesExistingPresentationImmediately()
  {
    mCallback.onSurfaceAvailable(mSurface);
    final CarPresentation presentation = mPresentations.constructed().get(0);
    clearInvocations(presentation);

    mCallback.setCurrentSpeed(54, "km/h");
    mCallback.setSpeedLimit(50);
    mCallback.setAlertForced(true);
    mCallback.setAlertForced(false);

    verify(presentation).setCurrentSpeed(54, "km/h");
    verify(presentation).setSpeedLimit(50);
    verify(presentation).setAlertForced(true);
    verify(presentation).setAlertForced(false);
    verifyNoMoreInteractions(presentation);
  }

  @Test
  public void replaysLatestStateWhenPresentationIsCreated()
  {
    mCallback.setCurrentSpeed(20, "km/h");
    mCallback.setSpeedLimit(30);
    mCallback.setCurrentSpeed(54, "km/h");
    mCallback.setSpeedLimit(50);
    mCallback.setAlertForced(true);
    assertEquals(0, mPresentations.constructed().size());

    mCallback.onSurfaceAvailable(mSurface);
    final CarPresentation presentation = mPresentations.constructed().get(0);
    final InOrder replay = inOrder(presentation);
    replay.verify(presentation).create();
    replay.verify(presentation).setCurrentSpeed(54, "km/h");
    replay.verify(presentation).setSpeedLimit(50);
    replay.verify(presentation).setAlertForced(true);
    verify(presentation, never()).setCurrentSpeed(20, "km/h");
    verify(presentation, never()).setSpeedLimit(30);
  }

  @Test
  public void replaysStateAfterPresentationRecreationWithoutLocationUpdate()
  {
    mCallback.setCurrentSpeed(64, "km/h");
    mCallback.setSpeedLimit(50);
    mCallback.setAlertForced(true);
    mCallback.onSurfaceAvailable(mSurface);
    final CarPresentation first = mPresentations.constructed().get(0);
    mCallback.destroy();
    verify(first).destroy();
    mCallback.onSurfaceAvailable(mSurface);

    assertEquals(2, mPresentations.constructed().size());
    final CarPresentation second = mPresentations.constructed().get(1);
    assertNotSame(first, second);
    final InOrder replay = inOrder(second);
    replay.verify(second).create();
    replay.verify(second).setCurrentSpeed(64, "km/h");
    replay.verify(second).setSpeedLimit(50);
    replay.verify(second).setAlertForced(true);
  }

  @Test
  public void retainsUpdatesMadeWhilePresentationIsAbsent()
  {
    mCallback.setCurrentSpeed(64, "km/h");
    mCallback.setSpeedLimit(50);
    mCallback.setAlertForced(true);
    mCallback.onSurfaceAvailable(mSurface);
    mCallback.destroy();
    mCallback.setCurrentSpeed(42, "mph");
    mCallback.setSpeedLimit(40);
    mCallback.setAlertForced(false);
    mCallback.onSurfaceAvailable(mSurface);

    final CarPresentation second = mPresentations.constructed().get(1);
    verify(second).setCurrentSpeed(42, "mph");
    verify(second).setSpeedLimit(40);
    verify(second).setAlertForced(false);
    verify(second, never()).setAlertForced(true);
    verify(second, never()).setCurrentSpeed(64, "km/h");
    verify(second, never()).setSpeedLimit(50);
  }

  @Test
  public void clearingLimitWhileAbsentDoesNotRestoreStaleLimit()
  {
    mCallback.setCurrentSpeed(64, "km/h");
    mCallback.setSpeedLimit(50);
    mCallback.onSurfaceAvailable(mSurface);
    mCallback.destroy();
    mCallback.setSpeedLimit(null);
    mCallback.onSurfaceAvailable(mSurface);

    final CarPresentation second = mPresentations.constructed().get(1);
    verify(second).setCurrentSpeed(64, "km/h");
    verify(second).setSpeedLimit(null);
    verify(second, never()).setSpeedLimit(50);
  }

  @Test
  public void initialPresentationHasNoKnownLimit()
  {
    mCallback.onSurfaceAvailable(mSurface);
    verify(mPresentations.constructed().get(0)).setSpeedLimit(null);
    verify(mPresentations.constructed().get(0)).setAlertForced(false);
  }

  @Test
  public void cameraAlertIsReplayedWithoutKnownRoadLimit()
  {
    mCallback.setAlertForced(true);
    mCallback.onSurfaceAvailable(mSurface);
    final CarPresentation presentation = mPresentations.constructed().get(0);
    verify(presentation).setSpeedLimit(null);
    verify(presentation).setAlertForced(true);
  }

  @Test
  public void clearingAlertBeforeCreationDoesNotReplayStaleAlert()
  {
    mCallback.setAlertForced(true);
    mCallback.setAlertForced(false);
    mCallback.onSurfaceAvailable(mSurface);
    final CarPresentation presentation = mPresentations.constructed().get(0);
    verify(presentation).setAlertForced(false);
    verify(presentation, never()).setAlertForced(true);
  }
}
