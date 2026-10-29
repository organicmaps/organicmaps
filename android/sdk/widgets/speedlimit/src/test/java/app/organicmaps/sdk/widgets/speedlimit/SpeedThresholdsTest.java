package app.organicmaps.sdk.widgets.speedlimit;

import static org.junit.Assert.assertEquals;

import org.junit.Test;

public class SpeedThresholdsTest
{
  private final SpeedThresholds mThresholds = SpeedThresholds.DEFAULT;

  @Test
  public void entryRequiresMoreThanTheThreshold()
  {
    assertEquals(Mode.Normal, mThresholds.modeFor(50, 50));
    assertEquals(Mode.Normal, mThresholds.modeFor(55, 50));
    assertEquals(Mode.Warning, mThresholds.modeFor(56, 50));
    assertEquals(Mode.Warning, mThresholds.modeFor(60, 50));
    assertEquals(Mode.Alert, mThresholds.modeFor(61, 50));
    assertEquals(Mode.Warning, mThresholds.modeFor(60, 50, Mode.Warning));
    assertEquals(Mode.Alert, mThresholds.modeFor(61, 50, Mode.Warning));
  }

  @Test
  public void warningClearsAtOrBelowItsResetBoundary()
  {
    assertEquals(Mode.Warning, mThresholds.modeFor(55, 50, Mode.Warning));
    assertEquals(Mode.Warning, mThresholds.modeFor(54, 50, Mode.Warning));
    assertEquals(Mode.Normal, mThresholds.modeFor(53, 50, Mode.Warning));
    assertEquals(Mode.Normal, mThresholds.modeFor(52, 50, Mode.Warning));
  }

  @Test
  public void alertClearsAtOrBelowItsResetBoundary()
  {
    assertEquals(Mode.Alert, mThresholds.modeFor(60, 50, Mode.Alert));
    assertEquals(Mode.Alert, mThresholds.modeFor(58, 50, Mode.Alert));
    assertEquals(Mode.Warning, mThresholds.modeFor(57, 50, Mode.Alert));
    assertEquals(Mode.Warning, mThresholds.modeFor(56, 50, Mode.Alert));
    assertEquals(Mode.Warning, mThresholds.modeFor(54, 50, Mode.Alert));
    assertEquals(Mode.Normal, mThresholds.modeFor(53, 50, Mode.Alert));
  }

  @Test
  public void unknownLimitsResetEveryMode()
  {
    for (Mode previous : Mode.values())
    {
      assertEquals(Mode.Normal, mThresholds.modeFor(100, null, previous));
      assertEquals(Mode.Normal, mThresholds.modeFor(100, 0, previous));
      assertEquals(Mode.Normal, mThresholds.modeFor(100, -1, previous));
    }
  }

  @Test
  public void zeroHysteresisUsesEntryBoundariesForReset()
  {
    SpeedThresholds thresholds = new SpeedThresholds(5, 10);
    for (Mode previous : Mode.values())
    {
      assertEquals(Mode.Normal, thresholds.modeFor(55, 50, previous));
      assertEquals(Mode.Warning, thresholds.modeFor(56, 50, previous));
      assertEquals(Mode.Warning, thresholds.modeFor(60, 50, previous));
      assertEquals(Mode.Alert, thresholds.modeFor(61, 50, previous));
    }
  }
}
