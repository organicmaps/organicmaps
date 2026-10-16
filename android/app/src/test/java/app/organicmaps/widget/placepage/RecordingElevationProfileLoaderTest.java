package app.organicmaps.widget.placepage;

import static org.junit.Assert.*;

import app.organicmaps.sdk.bookmarks.data.ElevationInfo;
import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.List;
import java.util.Queue;
import java.util.concurrent.Executor;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;
import org.junit.Test;

public class RecordingElevationProfileLoaderTest
{
  private static class Tasks implements Executor
  {
    final Queue<Runnable> pending = new ArrayDeque<>();

    @Override
    public void execute(Runnable task)
    {
      pending.add(task);
    }

    void runNext()
    {
      pending.remove().run();
    }
  }

  private static ElevationInfo data()
  {
    return new ElevationInfo(new ElevationInfo.Point[] {new ElevationInfo.Point(0, 100)}, 0, new double[0]);
  }

  @Test
  public void hiddenUpdatesDoNoWorkAndVisibleUpdatesCoalesce()
  {
    Tasks worker = new Tasks();
    Tasks ui = new Tasks();
    AtomicInteger calls = new AtomicInteger();
    List<ElevationInfo> displayed = new ArrayList<>();
    RecordingElevationProfileLoader loader =
        new RecordingElevationProfileLoader(worker, ui, () -> true, (previous, cancelled) -> {
          calls.incrementAndGet();
          return previous == null ? data() : previous;
        }, displayed::add);
    for (int i = 0; i < 100; ++i)
    {
      loader.invalidate();
      loader.setVisible(false);
    }
    assertTrue(worker.pending.isEmpty());
    assertEquals(0, calls.get());

    loader.setVisible(true);
    for (int i = 0; i < 100; ++i)
    {
      loader.invalidate();
      loader.setVisible(true);
    }
    assertEquals(1, worker.pending.size());
    worker.runNext();
    ui.runNext();
    assertEquals(1, displayed.size());
    assertEquals(1, worker.pending.size());
    worker.runNext();
    ui.runNext();
    assertEquals(2, calls.get());
    assertEquals(1, displayed.size()); // The same revision is neither converted nor redrawn again.
    assertTrue(worker.pending.isEmpty());
    loader.setVisible(false);
    loader.setVisible(true);
    assertTrue(worker.pending.isEmpty());
  }

  @Test
  public void hidingCancelsQueuedWorkAndShowingNeedsNoNewGpsFix()
  {
    Tasks worker = new Tasks();
    Tasks ui = new Tasks();
    AtomicInteger calls = new AtomicInteger();
    List<ElevationInfo> displayed = new ArrayList<>();
    RecordingElevationProfileLoader loader =
        new RecordingElevationProfileLoader(worker, ui, () -> true, (previous, cancelled) -> {
          calls.incrementAndGet();
          return data();
        }, displayed::add);
    loader.setVisible(true);
    loader.setVisible(false);
    worker.runNext();
    assertEquals(0, calls.get());
    assertTrue(ui.pending.isEmpty());
    loader.setVisible(true);
    worker.runNext();
    ui.runNext();
    assertEquals(1, calls.get());
    assertEquals(1, displayed.size());
  }

  @Test
  public void staleCompletionsCannotReplaceAReopenedGraph()
  {
    Tasks worker = new Tasks();
    Tasks ui = new Tasks();
    List<AtomicBoolean> cancellations = new ArrayList<>();
    List<ElevationInfo> loaded = new ArrayList<>();
    List<ElevationInfo> displayed = new ArrayList<>();
    RecordingElevationProfileLoader loader =
        new RecordingElevationProfileLoader(worker, ui, () -> true, (previous, cancelled) -> {
          cancellations.add(cancelled);
          ElevationInfo value = data();
          loaded.add(value);
          return value;
        }, displayed::add);
    loader.setVisible(true);
    worker.runNext();
    loader.setVisible(false);
    assertTrue(cancellations.get(0).get());
    loader.setVisible(true);
    ui.runNext();
    assertTrue(displayed.isEmpty());
    worker.runNext();
    ui.runNext();
    assertEquals(1, displayed.size());
    assertSame(loaded.get(1), displayed.get(0));
  }

  @Test
  public void emptySnapshotClearsThePreviousRecording()
  {
    Tasks worker = new Tasks();
    Tasks ui = new Tasks();
    AtomicInteger calls = new AtomicInteger();
    List<ElevationInfo> displayed = new ArrayList<>();
    RecordingElevationProfileLoader loader = new RecordingElevationProfileLoader(
        worker, ui, () -> true, (previous, cancelled) -> calls.incrementAndGet() == 1 ? data() : null, displayed::add);
    loader.setVisible(true);
    worker.runNext();
    ui.runNext();
    loader.invalidate();
    loader.setVisible(true);
    worker.runNext();
    ui.runNext();
    assertEquals(2, displayed.size());
    assertNull(displayed.get(1));
  }
  @Test
  public void visibilityIsCheckedAgainBeforePublishing()
  {
    Tasks worker = new Tasks();
    Tasks ui = new Tasks();
    AtomicBoolean visible = new AtomicBoolean(true);
    List<ElevationInfo> displayed = new ArrayList<>();
    RecordingElevationProfileLoader loader =
        new RecordingElevationProfileLoader(worker, ui, visible::get, (previous, cancelled) -> data(), displayed::add);
    loader.setVisible(true);
    worker.runNext();
    visible.set(false);
    ui.runNext();
    assertTrue(displayed.isEmpty());
    visible.set(true);
    loader.setVisible(true);
    worker.runNext();
    ui.runNext();
    assertEquals(1, displayed.size());
  }
}
