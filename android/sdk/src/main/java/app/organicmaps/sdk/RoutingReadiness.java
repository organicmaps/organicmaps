package app.organicmaps.sdk;

import androidx.annotation.NonNull;
import app.organicmaps.sdk.util.concurrency.UiThread;
import java.util.LinkedHashSet;
import java.util.Objects;
import java.util.Set;

/** Process-wide native routing readiness and cancellable one-shot tasks for main-thread hosts. */
@androidx.annotation.UiThread
public final class RoutingReadiness
{
  private static final RoutingReadiness sInstance = new RoutingReadiness();
  private final Set<Runnable> mTasks = new LinkedHashSet<>();
  private boolean mReady;

  RoutingReadiness() {}

  /** Returns the SDK-owned readiness shared by every routing controller and host. */
  public static RoutingReadiness get()
  {
    return sInstance;
  }

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

  void complete(Runnable onComplete)
  {
    checkThread();
    if (mReady)
      throw new IllegalStateException("Routing initialization completed twice");
    mReady = true;
    onComplete.run();
    // A callback can cancel another host's task while completion is being delivered.
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
      throw new IllegalStateException("Routing readiness must be changed on the main thread");
  }
}
