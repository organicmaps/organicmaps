package app.organicmaps.widget.placepage;

import static org.junit.Assert.assertEquals;

import org.junit.Test;

public class DirectionFragmentTest
{
  @Test
  public void wholeDegreeBearingWrapsAtNorth()
  {
    final double[] bearings = {359.4, 359.5, 359.6, 0, 0.4, 0.5, 0.6, 90, 180, 270};
    final String[] expected = {"359°", "0°", "0°", "0°", "0°", "1°", "1°", "90°", "180°", "270°"};
    for (int i = 0; i < bearings.length; i++)
      assertEquals(expected[i], DirectionFragment.formatAzimuth(Math.toRadians(bearings[i])));
  }
}
