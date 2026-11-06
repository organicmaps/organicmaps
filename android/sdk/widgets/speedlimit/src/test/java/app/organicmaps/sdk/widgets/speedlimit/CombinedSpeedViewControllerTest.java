package app.organicmaps.sdk.widgets.speedlimit;

import static org.mockito.Mockito.clearInvocations;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyNoInteractions;
import static org.mockito.Mockito.verifyNoMoreInteractions;
import static org.mockito.Mockito.when;

import org.junit.Before;
import org.junit.Test;

public class CombinedSpeedViewControllerTest
{
  private CombinedSpeedView mView;
  private CombinedSpeedViewController mController;

  @Before
  public void setUp()
  {
    mView = mock(CombinedSpeedView.class);
    mController = new CombinedSpeedViewController(mView, new SpeedThresholds(5, 10, 2, 3));
    mController.setSpeedLimit(50);
    clearInvocations(mView);
  }

  @Test
  public void customEntryAndResetBoundaries()
  {
    setSpeed(55);
    assertMode(Mode.Normal);
    setSpeed(56);
    assertMode(Mode.Warning);
    setSpeed(60);
    assertMode(Mode.Warning);
    setSpeed(61);
    assertMode(Mode.Alert);
    setSpeed(58);
    assertMode(Mode.Alert);
    setSpeed(57);
    assertMode(Mode.Warning);
    setSpeed(54);
    assertMode(Mode.Warning);
    setSpeed(53);
    assertMode(Mode.Normal);
  }

  @Test
  public void changedLimitResetsHysteresisAndReevaluatesCurrentSpeed()
  {
    setSpeed(62);
    assertMode(Mode.Alert);
    mController.setSpeedLimit(54);
    assertMode(Mode.Warning);
    mController.setSpeedLimit(57);
    assertMode(Mode.Normal);
    mController.setSpeedLimit(50);
    assertMode(Mode.Alert);
  }

  @Test
  public void unchangedLimitPreservesHysteresis()
  {
    setSpeed(61);
    assertMode(Mode.Alert);
    setSpeed(58);
    assertMode(Mode.Alert);
    mController.setSpeedLimit(50);
    verifyNoInteractions(mView);
    setSpeed(58);
    assertMode(Mode.Alert);
  }

  @Test
  public void changedUnitsResetHysteresis()
  {
    setSpeed(61);
    assertMode(Mode.Alert);
    setSpeed(58);
    assertMode(Mode.Alert);
    when(mView.getUnits()).thenReturn("km/h");
    mController.setUnits("mph");
    verify(mView).setUnits("mph");
    assertMode(Mode.Warning);
  }

  @Test
  public void unchangedUnitsPreserveHysteresis()
  {
    setSpeed(61);
    assertMode(Mode.Alert);
    setSpeed(58);
    assertMode(Mode.Alert);
    when(mView.getUnits()).thenReturn("km/h");
    mController.setUnits("km/h");
    verify(mView).getUnits();
    verifyNoMoreInteractions(mView);
    setSpeed(58);
    assertMode(Mode.Alert);
  }

  @Test
  public void changedUnitsPreserveCameraAlert()
  {
    setSpeed(58);
    assertMode(Mode.Warning);
    mController.setAlertForced(true);
    assertMode(Mode.Alert);
    when(mView.getUnits()).thenReturn("km/h");
    mController.setUnits("mph");
    assertMode(Mode.Alert);
    mController.setAlertForced(false);
    assertMode(Mode.Warning);
  }

  @Test
  public void unknownLimitsResetModeAndKnownLimitStartsWithoutHysteresis()
  {
    setSpeed(61);
    assertMode(Mode.Alert);
    mController.setSpeedLimit(null);
    assertMode(Mode.Normal);
    mController.setSpeedLimit(0);
    assertMode(Mode.Normal);
    mController.setSpeedLimit(-1);
    assertMode(Mode.Normal);
    setSpeed(64);
    assertMode(Mode.Normal);
    mController.setSpeedLimit(60);
    assertMode(Mode.Normal);
    setSpeed(66);
    assertMode(Mode.Warning);
  }

  @Test
  public void hidingAndShowingLimitResetsHysteresis()
  {
    setSpeed(61);
    assertMode(Mode.Alert);
    setSpeed(58);
    assertMode(Mode.Alert);
    mController.hideSpeedLimit();
    verify(mView).setSpeedLimit(null);
    assertMode(Mode.Normal);
    mController.showSpeedLimit();
    verify(mView).setSpeedLimit(50);
    assertMode(Mode.Warning);
  }

  @Test
  public void cameraAlertClearsToNormal()
  {
    setSpeed(54);
    assertMode(Mode.Normal);
    mController.setAlertForced(true);
    assertMode(Mode.Alert);
    mController.setAlertForced(false);
    assertMode(Mode.Normal);
  }

  @Test
  public void cameraAlertDoesNotActivateRoadAlertHysteresis()
  {
    setSpeed(58);
    assertMode(Mode.Warning);
    mController.setAlertForced(true);
    assertMode(Mode.Alert);
    mController.setAlertForced(false);
    assertMode(Mode.Warning);
  }

  @Test
  public void roadWarningHysteresisUpdatesDuringCameraAlert()
  {
    setSpeed(50);
    assertMode(Mode.Normal);
    mController.setAlertForced(true);
    assertMode(Mode.Alert);
    setSpeed(56);
    assertMode(Mode.Alert);
    setSpeed(54);
    assertMode(Mode.Alert);
    mController.setAlertForced(false);
    assertMode(Mode.Warning);
    setSpeed(53);
    assertMode(Mode.Normal);
  }

  @Test
  public void cameraAlertWorksWithUnknownRoadLimit()
  {
    setSpeed(50);
    assertMode(Mode.Normal);
    mController.setSpeedLimit(null);
    assertMode(Mode.Normal);
    mController.setAlertForced(true);
    assertMode(Mode.Alert);
    setSpeed(60);
    assertMode(Mode.Alert);
    mController.setAlertForced(false);
    assertMode(Mode.Normal);
  }

  @Test
  public void roadAlertRemainsWhenCameraAlertClears()
  {
    setSpeed(61);
    assertMode(Mode.Alert);
    mController.setAlertForced(true);
    assertMode(Mode.Alert);
    mController.setAlertForced(false);
    assertMode(Mode.Alert);
  }

  @Test
  public void unavailableSpeedClearsRoadAndCameraAlertsAndRecovers()
  {
    setSpeed(61);
    assertMode(Mode.Alert);
    mController.setAlertForced(true);
    assertMode(Mode.Alert);
    for (Integer speed : new Integer[] {null, -1, 1000, Integer.MAX_VALUE})
    {
      setSpeed(speed);
      assertMode(Mode.Normal);
    }
    setSpeed(60);
    assertMode(Mode.Alert);
    mController.setAlertForced(false);
    assertMode(Mode.Warning);
  }

  private void setSpeed(Integer speed)
  {
    when(mView.getSpeed()).thenReturn(speed);
    mController.setSpeed(speed);
    verify(mView).setSpeed(speed);
  }

  private void assertMode(Mode mode)
  {
    verify(mView).setMode(mode);
    clearInvocations(mView);
  }
}
