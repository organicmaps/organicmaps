package app.organicmaps.car.screens;

import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.app.Application;
import app.organicmaps.NavigationNativeShadows;
import app.organicmaps.sdk.car.renderer.Renderer;
import app.organicmaps.sdk.routing.CarDirection;
import app.organicmaps.sdk.routing.PedestrianDirection;
import app.organicmaps.sdk.routing.RoutingInfo;
import java.lang.reflect.Constructor;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.annotation.Config;

@RunWith(RobolectricTestRunner.class)
@Config(application = Application.class, manifest = Config.NONE, sdk = 33,
        shadows = NavigationNativeShadows.SpeedFormatting.class)
public class NavigationScreenTest
{
  private Renderer mRenderer;
  private RoutingInfo mInfo;

  @Before
  public void setUp()
  {
    mRenderer = mock(Renderer.class);
    mInfo = mock(RoutingInfo.class);
  }

  @Test
  public void cameraAlertDoesNotRequireKnownRoadLimit()
  {
    when(mInfo.isSpeedCamLimitExceeded()).thenReturn(true);
    NavigationScreen.updateSpeedLimit(mRenderer, true, mInfo);
    verify(mRenderer).setSpeedLimit(null);
    verify(mRenderer).setAlertForced(true);
  }

  @Test
  public void clearedCameraFlagClearsForcedAlert()
  {
    NavigationScreen.updateSpeedLimit(mRenderer, true, mInfo);
    verify(mRenderer).setAlertForced(false);
  }

  @Test
  public void missingRoutingInfoClearsForcedAlert()
  {
    NavigationScreen.updateSpeedLimit(mRenderer, true, null);
    verify(mRenderer).setAlertForced(false);
  }

  @Test
  public void stoppedNavigationClearsForcedAlert()
  {
    when(mInfo.isSpeedCamLimitExceeded()).thenReturn(true);
    NavigationScreen.updateSpeedLimit(mRenderer, false, mInfo);
    verify(mRenderer).setAlertForced(false);
  }

  @Test
  public void currentSpeedAndLimitUseTheSameProcessedSnapshot() throws ReflectiveOperationException
  {
    final Constructor<?> constructor = RoutingInfo.class.getDeclaredConstructors()[0];
    constructor.setAccessible(true);
    final RoutingInfo info = (RoutingInfo) constructor.newInstance(
        null, null, "", "", null, "", null, 0.0, CarDirection.NoTurn, CarDirection.NoTurn, PedestrianDirection.NoTurn,
        0, 0, null, 12.0, 8.0, true, false);
    NavigationScreen.updateSpeedLimit(mRenderer, true, info);
    verify(mRenderer).setCurrentSpeed(43, "km/h");
    verify(mRenderer).setSpeedLimit(29);
    verify(mRenderer).setAlertForced(true);
  }
}
