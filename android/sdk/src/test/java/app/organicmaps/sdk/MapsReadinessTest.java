package app.organicmaps.sdk;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertThrows;
import static org.junit.Assert.assertTrue;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.atomic.AtomicReference;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;

@RunWith(RobolectricTestRunner.class)
public class MapsReadinessTest
{
  @Test
  public void waitingTasksSeeReadyAndRunInRegistrationOrder()
  {
    final MapsReadiness readiness = new MapsReadiness();
    final List<String> events = new ArrayList<>();
    readiness.runWhenReady(() -> {
      assertTrue(readiness.isReady());
      events.add("phone");
    });
    readiness.runWhenReady(() -> events.add("car"));
    assertFalse(readiness.isReady());
    assertTrue(events.isEmpty());
    readiness.complete();
    assertEquals(List.of("phone", "car"), events);
    readiness.runWhenReady(() -> events.add("late"));
    assertEquals(List.of("phone", "car", "late"), events);
    assertThrows(IllegalStateException.class, readiness::complete);
  }

  @Test
  public void recreatedHostReplacesItsCancelledTaskAndRepeatedRegistrationRunsOnce()
  {
    final MapsReadiness readiness = new MapsReadiness();
    final List<String> events = new ArrayList<>();
    final Runnable old = () -> events.add("destroyed");
    final Runnable replacement = () -> events.add("replacement");
    readiness.runWhenReady(old);
    assertTrue(readiness.cancel(old));
    readiness.runWhenReady(replacement);
    readiness.runWhenReady(replacement);
    readiness.complete();
    assertEquals(List.of("replacement"), events);
  }

  @Test
  public void completionCanCancelAnotherHostsQueuedTask()
  {
    final MapsReadiness readiness = new MapsReadiness();
    final List<String> events = new ArrayList<>();
    final Runnable destroyed = () -> events.add("destroyed");
    readiness.runWhenReady(() -> {
      events.add("first");
      readiness.cancel(destroyed);
    });
    readiness.runWhenReady(destroyed);
    readiness.complete();
    assertEquals(List.of("first"), events);
  }

  @Test
  public void aQueuedTaskRegisteredAgainDuringDispatchRunsOnce()
  {
    final MapsReadiness readiness = new MapsReadiness();
    final List<String> events = new ArrayList<>();
    final Runnable task = () -> events.add("task");
    readiness.runWhenReady(() -> readiness.runWhenReady(task));
    readiness.runWhenReady(task);
    readiness.complete();
    assertEquals(List.of("task"), events);
  }

  @Test
  public void nullTasksFailAtRegistrationWithoutBreakingCompletion()
  {
    final MapsReadiness readiness = new MapsReadiness();
    assertThrows(NullPointerException.class, () -> readiness.runWhenReady(null));
    final List<String> events = new ArrayList<>();
    readiness.runWhenReady(() -> events.add("ready"));
    readiness.complete();
    assertEquals(List.of("ready"), events);
  }

  @Test
  public void anotherThreadCannotMutateReadiness() throws InterruptedException
  {
    final MapsReadiness readiness = new MapsReadiness();
    final AtomicReference<Throwable> failure = new AtomicReference<>();
    final Thread other = new Thread(() -> {
      try
      {
        readiness.complete();
      }
      catch (Throwable error)
      {
        failure.set(error);
      }
    });
    other.start();
    other.join();
    assertTrue(failure.get() instanceof IllegalStateException);
    assertFalse(readiness.isReady());
  }
}
