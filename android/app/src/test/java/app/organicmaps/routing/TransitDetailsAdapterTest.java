package app.organicmaps.routing;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.doNothing;
import static org.mockito.Mockito.spy;

import app.organicmaps.sdk.routing.TransitStepInfo;
import app.organicmaps.sdk.routing.TransitStepType;
import java.lang.reflect.Constructor;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.List;
import org.junit.Before;
import org.junit.Test;

/**
 * Verifies how {@link TransitDetailsAdapter} maps the raw transit steps onto rows: ruler segments
 * are dropped, intermediate (Add Stop) points get their own row, walks and intermediate points are not
 * rides, and only rides sharing a stop name are drawn as a joined transfer.
 *
 * The adapter is a {@link androidx.recyclerview.widget.RecyclerView.Adapter}, so it is spied and its
 * {@code notifyDataSetChanged()} stubbed (matching PlaceOpeningHoursAdapterTest); the private
 * classification helpers are reached via reflection.
 */
public class TransitDetailsAdapterTest
{
  private TransitDetailsAdapter mAdapter;
  private int mTypeWalk;
  private int mTypeRide;
  private int mTypeIntermediate;
  private Method mIsRide;
  private Method mIsSameStopTransfer;

  @Before
  public void setUp() throws Exception
  {
    mAdapter = spy(new TransitDetailsAdapter());
    doNothing().when(mAdapter).notifyDataSetChanged();

    mTypeWalk = intConstant("TYPE_WALK");
    mTypeRide = intConstant("TYPE_RIDE");
    mTypeIntermediate = intConstant("TYPE_INTERMEDIATE");

    mIsRide = TransitDetailsAdapter.class.getDeclaredMethod("isRide", int.class);
    mIsRide.setAccessible(true);
    mIsSameStopTransfer = TransitDetailsAdapter.class.getDeclaredMethod("isSameStopTransfer", int.class);
    mIsSameStopTransfer.setAccessible(true);
  }

  @Test
  public void rulerSteps_areDropped() throws Exception
  {
    mAdapter.setItems(List.of(walk(), ruler(), ride("U5")));

    assertEquals(2, mAdapter.getItemCount());
    assertEquals(mTypeWalk, mAdapter.getItemViewType(0));
    assertEquals(mTypeRide, mAdapter.getItemViewType(1));
  }

  @Test
  public void intermediatePoint_isKeptAsItsOwnRowBetweenWalks() throws Exception
  {
    // walk -> Add Stop -> walk -> ride: the waypoint separates its two walking legs.
    mAdapter.setItems(List.of(walk(), intermediate(0), walk(), ride("U5")));

    assertEquals(4, mAdapter.getItemCount());
    assertEquals(mTypeWalk, mAdapter.getItemViewType(0));
    assertEquals(mTypeIntermediate, mAdapter.getItemViewType(1));
    assertEquals(mTypeWalk, mAdapter.getItemViewType(2));
    assertEquals(mTypeRide, mAdapter.getItemViewType(3));
  }

  @Test
  public void isRide_excludesWalksAndIntermediatePoints() throws Exception
  {
    mAdapter.setItems(List.of(walk(), intermediate(0), ride("U5")));

    assertFalse(isRide(0)); // walk
    assertFalse(isRide(1)); // intermediate point
    assertTrue(isRide(2)); // transit ride
    assertFalse(isRide(-1)); // out of bounds
    assertFalse(isRide(3)); // out of bounds
  }

  @Test
  public void sameStopTransfer_joinsRidesSharingTheStop() throws Exception
  {
    mAdapter.setItems(List.of(ride("U5", "A", "B"), ride("U6", "B", "C")));

    assertFalse(isSameStopTransfer(0));
    assertTrue(isSameStopTransfer(1));
  }

  @Test
  public void transferBetweenDifferentlyNamedStops_keepsBothNames() throws Exception
  {
    mAdapter.setItems(List.of(ride("U5", "A", "Incoming station"), ride("U6", "Connecting station", "C")));

    assertFalse(isSameStopTransfer(1));
  }

  @Test
  public void walkBetweenRides_isNotATransfer() throws Exception
  {
    mAdapter.setItems(List.of(ride("U5", "A", "B"), walk(), ride("U6", "B", "C")));

    assertFalse(isSameStopTransfer(2));
  }

  // --- helpers ---

  private boolean isSameStopTransfer(int position) throws Exception
  {
    return (boolean) mIsSameStopTransfer.invoke(mAdapter, position);
  }

  private boolean isRide(int position) throws Exception
  {
    return (boolean) mIsRide.invoke(mAdapter, position);
  }

  private static int intConstant(String name) throws Exception
  {
    Field field = TransitDetailsAdapter.class.getDeclaredField(name);
    field.setAccessible(true);
    return field.getInt(null);
  }

  private static TransitStepInfo walk() throws Exception
  {
    return step(TransitStepType.PEDESTRIAN, null, null, null);
  }

  private static TransitStepInfo ride(String number) throws Exception
  {
    return ride(number, null, null);
  }

  private static TransitStepInfo ride(String number, String startStop, String endStop) throws Exception
  {
    return step(TransitStepType.SUBWAY, number, startStop, endStop);
  }

  private static TransitStepInfo intermediate(int index)
  {
    return TransitStepInfo.intermediatePoint(index);
  }

  private static TransitStepInfo ruler()
  {
    return TransitStepInfo.ruler("1", "km");
  }

  // TransitStepInfo's constructor is private (JNI-only); build test instances reflectively rather
  // than widening the production API.
  private static TransitStepInfo step(TransitStepType type, String number, String startStop, String endStop)
      throws Exception
  {
    Constructor<TransitStepInfo> ctor = TransitStepInfo.class.getDeclaredConstructor(
        int.class, String.class, String.class, int.class, String.class, int.class, int.class, String.class,
        String.class, int.class, String[].class, String.class);
    ctor.setAccessible(true);
    return ctor.newInstance(type.ordinal(), null, null, 0, number, 0, 0, startStop, endStop, 0, null, number);
  }
}
