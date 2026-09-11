package app.organicmaps.util;

import static org.junit.Assert.assertEquals;

import org.junit.Test;

public class UtilsTest
{
  @Test
  public void routingTimeRoundsToNearestMinute()
  {
    assertEquals(0, Utils.roundRoutingTimeToMinutes(0));
    assertEquals(0, Utils.roundRoutingTimeToMinutes(29));
    assertEquals(1, Utils.roundRoutingTimeToMinutes(30));
    assertEquals(1, Utils.roundRoutingTimeToMinutes(89));
    assertEquals(2, Utils.roundRoutingTimeToMinutes(90));
    assertEquals(22, Utils.roundRoutingTimeToMinutes(1307));
    assertEquals(60, Utils.roundRoutingTimeToMinutes(3599));
    assertEquals(60, Utils.roundRoutingTimeToMinutes(3600));
  }
}
