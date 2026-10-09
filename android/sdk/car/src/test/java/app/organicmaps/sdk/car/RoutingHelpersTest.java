package app.organicmaps.sdk.car;

import static org.junit.Assert.assertEquals;

import androidx.car.app.navigation.model.Maneuver;
import app.organicmaps.sdk.routing.CarDirection;
import app.organicmaps.sdk.routing.RoundaboutDirection;
import org.junit.Test;

public class RoutingHelpersTest
{
  @Test
  public void mapsRoundaboutEntryAndExitWithAngle()
  {
    assertEquals(Maneuver.TYPE_ROUNDABOUT_ENTER_AND_EXIT_CCW_WITH_ANGLE,
                 type(CarDirection.EnterRoundAbout, 2, RoundaboutDirection.CounterClockwise, 180, true));
    assertEquals(Maneuver.TYPE_ROUNDABOUT_ENTER_AND_EXIT_CW_WITH_ANGLE,
                 type(CarDirection.EnterRoundAbout, 2, RoundaboutDirection.Clockwise, 180, true));
  }

  @Test
  public void preservesDirectionWithoutAngle()
  {
    assertEquals(Maneuver.TYPE_ROUNDABOUT_ENTER_AND_EXIT_CCW,
                 type(CarDirection.EnterRoundAbout, 2, RoundaboutDirection.CounterClockwise, 0, true));
    assertEquals(Maneuver.TYPE_ROUNDABOUT_ENTER_AND_EXIT_CW,
                 type(CarDirection.EnterRoundAbout, 2, RoundaboutDirection.Clockwise, 0, true));
    assertEquals(Maneuver.TYPE_ROUNDABOUT_ENTER_CCW,
                 type(CarDirection.EnterRoundAbout, 1, RoundaboutDirection.CounterClockwise, 0, false));
    assertEquals(Maneuver.TYPE_ROUNDABOUT_ENTER_CW,
                 type(CarDirection.EnterRoundAbout, 1, RoundaboutDirection.Clockwise, 0, false));
  }

  @Test
  public void omitsAngleWhenExitNumberIsUnavailable()
  {
    assertEquals(Maneuver.TYPE_ROUNDABOUT_ENTER_CCW,
                 type(CarDirection.EnterRoundAbout, 0, RoundaboutDirection.CounterClockwise, 180, true));
  }

  @Test
  public void mapsRoundaboutExitAsExitOnly()
  {
    assertEquals(Maneuver.TYPE_ROUNDABOUT_EXIT_CCW,
                 type(CarDirection.LeaveRoundAbout, 2, RoundaboutDirection.CounterClockwise, 180, true));
    assertEquals(Maneuver.TYPE_ROUNDABOUT_EXIT_CW,
                 type(CarDirection.LeaveRoundAbout, 2, RoundaboutDirection.Clockwise, 180, true));
  }

  @Test
  public void buildsCombinedAndExitOnlyManeuvers()
  {
    final Maneuver combined = build(CarDirection.EnterRoundAbout, 2, RoundaboutDirection.CounterClockwise, 180, true);
    assertEquals(2, combined.getRoundaboutExitNumber());
    assertEquals(180, combined.getRoundaboutExitAngle());

    final Maneuver exitOnly = build(CarDirection.LeaveRoundAbout, 2, RoundaboutDirection.CounterClockwise, 180, true);
    assertEquals(0, exitOnly.getRoundaboutExitNumber());
    assertEquals(0, exitOnly.getRoundaboutExitAngle());
  }

  @Test
  public void buildsManeuverWithoutExitNumber()
  {
    final Maneuver maneuver = build(CarDirection.EnterRoundAbout, 0, RoundaboutDirection.CounterClockwise, 0, true);
    assertEquals(Maneuver.TYPE_ROUNDABOUT_ENTER_CCW, maneuver.getType());
    assertEquals(0, maneuver.getRoundaboutExitNumber());
  }

  private static int type(CarDirection carDirection, int exitNum, RoundaboutDirection direction, int exitAngle,
                          boolean hasExit)
  {
    return RoutingHelpers.getRoundaboutManeuverType(carDirection, exitNum, direction, exitAngle, hasExit);
  }

  private static Maneuver build(CarDirection carDirection, int exitNum, RoundaboutDirection direction, int exitAngle,
                                boolean hasExit)
  {
    final int maneuverType = type(carDirection, exitNum, direction, exitAngle, hasExit);
    final Maneuver.Builder builder = new Maneuver.Builder(maneuverType);
    RoutingHelpers.setRoundaboutFields(builder, maneuverType, exitNum, exitAngle);
    return builder.build();
  }
}
