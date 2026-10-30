package app.organicmaps.routing;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import android.app.Instrumentation;
import androidx.test.platform.app.InstrumentationRegistry;
import app.organicmaps.MwmApplication;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.Router;
import app.organicmaps.sdk.routing.RouteMarkData;
import app.organicmaps.sdk.routing.RouteMarkType;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.routing.RoutingListener;
import java.io.IOException;
import java.io.UncheckedIOException;
import java.lang.reflect.Field;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.FutureTask;
import java.util.concurrent.TimeUnit;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;

public class RoutingControllerReorderTest
{
  private final Instrumentation mInstrumentation = InstrumentationRegistry.getInstrumentation();
  private final MwmApplication mApplication = MwmApplication.from(mInstrumentation.getTargetContext());
  private RoutingController mController;
  private Router mOriginalRouter;

  @Before
  public void initializeCore() throws Exception
  {
    final CountDownLatch initialized = new CountDownLatch(1);
    onMain(() -> {
      try
      {
        if (!mApplication.initOrganicMaps(initialized::countDown))
          initialized.countDown();
      }
      catch (IOException e)
      {
        throw new UncheckedIOException(e);
      }
    });
    assertTrue("Core initialization did not complete", initialized.await(30, TimeUnit.SECONDS));
    onMain(() -> {
      mOriginalRouter = Router.getLastUsed();
      Framework.nativeCloseRouting();
      mController = new RoutingController();
      mController.initialize(mApplication.getLocationHelper());
      mController.setRouterType(Router.Ruler);
    });
  }

  @After
  public void clearRoute() throws Exception
  {
    if (mController == null)
      return;
    onMain(() -> {
      Framework.nativeCloseRouting();
      Router.set(mOriginalRouter);
      RoutingController.get().initialize(mApplication.getLocationHelper());
    });
  }

  @Test
  public void acceptedPermutationsBuildOnceAndPreserveTheSelectedRouter() throws Exception
  {
    onMain(() -> {
      for (int count : new int[] {3, 102})
      {
        mController.cancel();
        seedRoute(count);
        mController.prepare(null, null, Router.Ruler);
        final List<RouteMarkData> original = List.of(Framework.nativeGetRoutePoints());
        final int[] order = new int[count];
        for (int i = 0; i < count; ++i)
          order[i] = count - 1 - i;
        final int[] builds = {0};
        mController.attach(new RoutingController.Container() {
          @Override
          public void onStartRouteBuilding()
          {
            ++builds[0];
          }
        });

        assertTrue(mController.reorderRoutePoints(original, ordered(original, order)));

        final RouteMarkData[] actual = Framework.nativeGetRoutePoints();
        assertEquals(count, actual.length);
        for (int i = 0; i < count; ++i)
        {
          assertEquals(original.get(order[i]).mTitle, actual[i].mTitle);
          assertEquals(original.get(order[i]).mLat, actual[i].mLat, 0);
          assertEquals(original.get(order[i]).mLon, actual[i].mLon, 0);
        }
        assertEquals(RouteMarkType.Start, actual[0].mPointType);
        assertEquals(RouteMarkType.Finish, actual[count - 1].mPointType);
        assertEquals(1, builds[0]);
        assertEquals(Router.Ruler, mController.getLastRouterType());
        assertTrue(mController.isBuilding());
        mController.detach();
      }
    });
  }

  @Test
  public void obsoleteSnapshotsCannotReplaceNativeEdits() throws Exception
  {
    onMain(() -> {
      seedRoute(3);
      mController.prepare(null, null, Router.Ruler);
      final List<RouteMarkData> original = List.of(Framework.nativeGetRoutePoints());
      assertTrue(Framework.nativeReverseRoutePoints());
      final RouteMarkData[] reversed = Framework.nativeGetRoutePoints();

      assertFalse(mController.reorderRoutePoints(original, ordered(original, 1, 0, 2)));

      final RouteMarkData[] actual = Framework.nativeGetRoutePoints();
      for (int i = 0; i < actual.length; ++i)
        assertTrue(reversed[i].hasSameData(actual[i]));
      assertFalse(mController.isBuilding());
    });
  }

  @Test
  public void anIdleControllerDoesNotCommitCapturedPoints() throws Exception
  {
    onMain(() -> {
      seedRoute(3);
      final List<RouteMarkData> original = List.of(Framework.nativeGetRoutePoints());
      assertFalse(mController.reorderRoutePoints(original, ordered(original, 2, 1, 0)));
      final RouteMarkData[] actual = Framework.nativeGetRoutePoints();
      for (int i = 0; i < actual.length; ++i)
        assertTrue(original.get(i).hasSameData(actual[i]));
    });
  }

  @Test
  public void unchangedOrderDoesNotBuild() throws Exception
  {
    onMain(() -> {
      seedRoute(3);
      mController.prepare(null, null, Router.Ruler);
      final List<RouteMarkData> original = List.of(Framework.nativeGetRoutePoints());
      assertFalse(mController.reorderRoutePoints(original, original));
      assertFalse(mController.isBuilding());
    });
  }

  @Test
  public void partialPlansRemainUnchanged() throws Exception
  {
    onMain(() -> {
      for (int count : new int[] {0, 1})
      {
        mController.cancel();
        seedRoute(count);
        mController.prepare(null, null, Router.Ruler);
        final int[] order = new int[count];
        final List<RouteMarkData> original = List.of(Framework.nativeGetRoutePoints());
        assertFalse(mController.reorderRoutePoints(original, original));
        assertFalse(Framework.nativeReorderRoutePoints(order));
        assertEquals(count, Framework.nativeGetRoutePoints().length);
        assertFalse(mController.isBuilding());
      }
    });
  }

  @Test
  public void aNewOrderDiscardsThePreviousCachedBuildError() throws Exception
  {
    final Field listener = RoutingController.class.getDeclaredField("mRoutingListener");
    listener.setAccessible(true);
    onMain(() -> {
      seedRoute(3);
      mController.prepare(null, null, Router.Ruler);
      mController.onSaveState();
      try
      {
        ((RoutingListener) listener.get(mController))
            .onRoutingEvent(app.organicmaps.sdk.routing.ResultCodes.ROUTE_NOT_FOUND, new String[0]);
      }
      catch (IllegalAccessException e)
      {
        throw new AssertionError(e);
      }
      final List<RouteMarkData> original = List.of(Framework.nativeGetRoutePoints());
      assertTrue(mController.reorderRoutePoints(original, ordered(original, 2, 1, 0)));
      final int[] errors = {0};
      mController.attach(new RoutingController.Container() {
        @Override
        public void onCommonBuildError(int code, String[] maps)
        {
          ++errors[0];
        }
      });
      mController.restore();
      assertEquals(0, errors[0]);
      assertTrue(mController.isBuilding());
    });
  }

  private static List<RouteMarkData> ordered(List<RouteMarkData> snapshot, int... indices)
  {
    final List<RouteMarkData> result = new ArrayList<>(indices.length);
    for (int index : indices)
      result.add(snapshot.get(index));
    return List.copyOf(result);
  }

  private static void seedRoute(int count)
  {
    for (int i = 0; i < count; ++i)
    {
      final RouteMarkType type = i == 0         ? RouteMarkType.Start
                               : i + 1 == count ? RouteMarkType.Finish
                                                : RouteMarkType.Intermediate;
      assertTrue(Framework.nativeAddRoutePoint("Point " + i, "", type, false, 1 + i * 0.001, 1 + i * 0.001, false));
    }
  }

  private void onMain(Runnable action) throws Exception
  {
    final FutureTask<Void> task = new FutureTask<>(action, null);
    mInstrumentation.runOnMainSync(task);
    task.get();
  }
}
