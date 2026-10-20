package app.organicmaps.sdk.car;

import static org.junit.Assert.assertEquals;

import android.content.pm.PackageManager;
import android.os.Parcel;
import androidx.car.app.navigation.model.LaneDirection;
import androidx.car.app.navigation.model.Step;
import androidx.car.app.serialization.Bundleable;
import androidx.test.platform.app.InstrumentationRegistry;
import app.organicmaps.sdk.routing.LaneInfo;
import app.organicmaps.sdk.routing.LaneWay;
import java.util.List;
import org.junit.Test;

public class RoutingUtilsInstrumentedTest
{
  @Test
  public void structuredLanesSurviveCarAppIpc() throws Exception
  {
    final PackageManager packageManager =
        InstrumentationRegistry.getInstrumentation().getTargetContext().getPackageManager();
    final boolean isAutomotive = packageManager.hasSystemFeature(PackageManager.FEATURE_AUTOMOTIVE);
    CarTypeHelper.setCarType(isAutomotive ? CarType.Automotive : CarType.AndroidAuto);

    final Step step = new Step.Builder()
                          .addLane(RoutingUtils.createLane(new LaneInfo(new LaneWay[] {LaneWay.None}, LaneWay.None)))
                          .addLane(RoutingUtils.createLane(new LaneInfo(new LaneWay[] {LaneWay.None}, LaneWay.Right)))
                          .build();
    final Step restored = roundTrip(step);
    assertEquals(List.of(LaneDirection.create(LaneDirection.SHAPE_UNKNOWN, false)),
                 restored.getLanes().get(0).getDirections());
    assertEquals(List.of(LaneDirection.create(LaneDirection.SHAPE_NORMAL_RIGHT, true)),
                 restored.getLanes().get(1).getDirections());
  }

  private static Step roundTrip(Step step) throws Exception
  {
    final Parcel parcel = Parcel.obtain();
    try
    {
      Bundleable.create(step).writeToParcel(parcel, 0);
      parcel.setDataPosition(0);
      return (Step) Bundleable.CREATOR.createFromParcel(parcel).get();
    }
    finally
    {
      parcel.recycle();
    }
  }
}
