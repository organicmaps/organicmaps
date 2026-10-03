package app.organicmaps.sdk.car;

import static org.junit.Assert.assertEquals;

import androidx.car.app.navigation.model.LaneDirection;
import app.organicmaps.sdk.routing.LaneInfo;
import app.organicmaps.sdk.routing.LaneWay;
import java.util.List;
import org.junit.After;
import org.junit.Test;

public class RoutingUtilsTest
{
  @After
  public void resetCarType()
  {
    CarTypeHelper.sCarType = null;
  }

  @Test
  public void rightTurnSelectsOnlyRightmostUnrestrictedLane()
  {
    for (final CarType carType : CarType.values())
    {
      CarTypeHelper.sCarType = carType;
      assertDirections(new LaneInfo(new LaneWay[] {LaneWay.Left}, LaneWay.None),
                       LaneDirection.create(LaneDirection.SHAPE_NORMAL_LEFT, false));
      assertDirections(new LaneInfo(new LaneWay[] {LaneWay.None}, LaneWay.None),
                       LaneDirection.create(LaneDirection.SHAPE_UNKNOWN, false));
      assertDirections(new LaneInfo(new LaneWay[] {LaneWay.None}, LaneWay.Right),
                       LaneDirection.create(LaneDirection.SHAPE_NORMAL_RIGHT, true));
    }
  }

  @Test
  public void leftTurnSelectsOnlyLeftmostUnrestrictedLane()
  {
    for (final CarType carType : CarType.values())
    {
      CarTypeHelper.sCarType = carType;
      assertDirections(new LaneInfo(new LaneWay[] {LaneWay.None}, LaneWay.Left),
                       LaneDirection.create(LaneDirection.SHAPE_NORMAL_LEFT, true));
      assertDirections(new LaneInfo(new LaneWay[0], LaneWay.Left),
                       LaneDirection.create(LaneDirection.SHAPE_NORMAL_LEFT, true));
      assertDirections(new LaneInfo(new LaneWay[] {LaneWay.None}, LaneWay.None),
                       LaneDirection.create(LaneDirection.SHAPE_UNKNOWN, false));
      assertDirections(new LaneInfo(new LaneWay[] {LaneWay.Right}, LaneWay.None),
                       LaneDirection.create(LaneDirection.SHAPE_NORMAL_RIGHT, false));
    }
  }

  @Test
  public void explicitDirectionsKeepTheirRecommendationsAndUnknownStaysUnselected()
  {
    for (final CarType carType : CarType.values())
    {
      CarTypeHelper.sCarType = carType;
      assertDirections(new LaneInfo(new LaneWay[] {LaneWay.Left, LaneWay.Through}, LaneWay.Through),
                       LaneDirection.create(LaneDirection.SHAPE_NORMAL_LEFT, false),
                       LaneDirection.create(LaneDirection.SHAPE_STRAIGHT, true));
      assertDirections(new LaneInfo(new LaneWay[] {LaneWay.None}, LaneWay.None),
                       LaneDirection.create(LaneDirection.SHAPE_UNKNOWN, false));
      assertDirections(new LaneInfo(new LaneWay[] {LaneWay.Right}, LaneWay.None),
                       LaneDirection.create(LaneDirection.SHAPE_NORMAL_RIGHT, false));
    }
  }

  private static void assertDirections(LaneInfo laneInfo, LaneDirection... expected)
  {
    assertEquals(List.of(expected), RoutingUtils.createLane(laneInfo).getDirections());
  }
}
