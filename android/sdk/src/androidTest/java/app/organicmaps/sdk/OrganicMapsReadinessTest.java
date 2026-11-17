package app.organicmaps.sdk;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertThrows;
import static org.junit.Assert.assertTrue;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;
import app.organicmaps.sdk.routing.RoutingController;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicReference;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class OrganicMapsReadinessTest
{
  @Test
  public void nativeCompletionPublishesReadinessAfterSetupAndOnlyNotifiesLiveHosts() throws Exception
  {
    final var instrumentation = InstrumentationRegistry.getInstrumentation();
    final CountDownLatch complete = new CountDownLatch(1);
    final AtomicReference<Throwable> failure = new AtomicReference<>();
    final List<String> events = new ArrayList<>();
    instrumentation.runOnMainSync(() -> {
      try
      {
        final OrganicMaps sdk =
            new OrganicMaps(instrumentation.getTargetContext(), "test", "app.organicmaps.sdk.test", 1, "test");
        final RoutingReadiness routing = RoutingReadiness.get();
        assertFalse(sdk.arePlatformAndCoreInitialized());
        assertFalse(sdk.isRoutingReady());
        assertThrows(NullPointerException.class, () -> sdk.init(null));
        assertFalse(sdk.arePlatformAndCoreInitialized());
        final Runnable oldPhone = () -> events.add("destroyed phone");
        routing.runWhenReady(oldPhone);
        routing.runWhenReady(() -> events.add("car"));
        assertTrue(sdk.init(() -> {
          try
          {
            assertTrue(sdk.arePlatformAndCoreInitialized());
            assertTrue(sdk.isRoutingReady());
            // Exercise the actual JNI backend in this callback, with its readiness precondition.
            Framework.nativeBuildRoute();
            final RoutingController controller = new RoutingController();
            controller.prepare(null, null, Router.Ruler);
            assertTrue(controller.isPlanning());
            controller.cancel();
            events.add("init");
          }
          catch (Throwable error)
          {
            failure.set(error);
          }
        }));
        // The main-loop native completion cannot run inside this synchronous setup call.
        assertTrue(sdk.arePlatformAndCoreInitialized());
        assertFalse(sdk.isRoutingReady());
        assertTrue(events.isEmpty());
        assertFalse(sdk.init(() -> events.add("ignored duplicate init")));
        routing.cancel(oldPhone);
        routing.runWhenReady(() -> {
          events.add("replacement phone");
          complete.countDown();
        });
      }
      catch (Throwable error)
      {
        failure.set(error);
        complete.countDown();
      }
    });
    assertTrue("Native routing initialization timed out", complete.await(60, TimeUnit.SECONDS));
    if (failure.get() != null)
      throw new AssertionError("SDK startup failed", failure.get());
    assertEquals(List.of("init", "car", "replacement phone"), events);
    instrumentation.runOnMainSync(() -> RoutingReadiness.get().runWhenReady(() -> events.add("late")));
    assertEquals(List.of("init", "car", "replacement phone", "late"), events);
  }
}
