package app.organicmaps.sdk;

import androidx.annotation.NonNull;
import app.organicmaps.sdk.util.concurrency.UiThread;
import java.util.LinkedHashSet;
import java.util.Objects;
import java.util.Set;

/**
 * Completion of map registration, search, editor and routing setup. Activities and car sessions can
 * register cancellable one-shot tasks on the main thread. Obtain it from {@link OrganicMaps#getMapsReadiness()}.
 */
@androidx.annotation.UiThread
public final class MapsReadiness
{
  private final Set<Runnable> mTasks = new LinkedHashSet<>();
  private boolean mReady;

  MapsReadiness() {}

  /** Returns whether asynchronous map registration and native routing initialization have completed. */
  public boolean isReady()
  {
    checkThread();
    return mReady;
  }

  /** Runs a task immediately if ready, otherwise retains it once until completion or cancellation. */
  public void runWhenReady(@NonNull Runnable task)
  {
    checkThread();
    Objects.requireNonNull(task);
    if (mReady)
    {
      // Completion callbacks can re-register a task that is still queued.
      mTasks.remove(task);
      task.run();
    }
    else
      mTasks.add(task);
  }

  /** Removes a pending task; a task already running is not interrupted. */
  public boolean cancel(@NonNull Runnable task)
  {
    checkThread();
    Objects.requireNonNull(task);
    return mTasks.remove(task);
  }

  void complete()
  {
    checkThread();
    if (mReady)
      throw new IllegalStateException("Maps loading completed twice");
    mReady = true;
    // A task can cancel another screen's or session's task while completion is being delivered.
    while (!mTasks.isEmpty())
    {
      final Runnable task = mTasks.iterator().next();
      mTasks.remove(task);
      task.run();
    }
  }

  private void checkThread()
  {
    if (!UiThread.isUiThread())
      throw new IllegalStateException("Maps readiness must be used on the main thread");
  }
}
