package app.organicmaps.sdk.location;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import org.junit.Test;

public class NoGpsStatusTest
{
  // The navigation without GPS keeps running in the background while the box tells the car data.
  @Test
  public void hasCarData_whileTheBoxTellsTheSpeed()
  {
    final NoGps.Status status = new NoGps.Status();
    status.inertialStarted = true;
    status.speedKmh = 0;
    assertTrue(status.hasCarData());
    status.speedKmh = 60;
    assertTrue(status.hasCarData());
  }

  // On a parking the box sleeps with the engine stopped: the speed is unknown.
  @Test
  public void hasNoCarData_withoutTheSpeedOrTheNavigation()
  {
    final NoGps.Status status = new NoGps.Status();
    status.inertialStarted = true;
    status.speedKmh = -1;
    assertFalse(status.hasCarData());
    status.inertialStarted = false;
    status.speedKmh = 60;
    assertFalse(status.hasCarData());
  }
}
