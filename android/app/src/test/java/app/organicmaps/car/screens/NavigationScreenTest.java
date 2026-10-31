package app.organicmaps.car.screens;

import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import app.organicmaps.sdk.car.renderer.Renderer;
import app.organicmaps.sdk.routing.RoutingInfo;
import org.junit.Before;
import org.junit.Test;

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
}
