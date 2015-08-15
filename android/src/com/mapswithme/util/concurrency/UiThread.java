package com.mapswithme.util.concurrency;

import android.os.Handler;
import android.os.Looper;

import com.mapswithme.util.Proc;

public class UiThread
{
  private static final Handler sUiHandler = new Handler(Looper.getMainLooper());

  /**
   * Helper runnable classes that can be scheduled at any thread but will be started on UI thread only.
   */
  public static abstract class UiRunnable implements Runnable
  {
    @Override
    public final void run()
    {
      if (currentThreadIsUi())
        runUi();
      else
        UiThread.run(this);
    }

    protected abstract void runUi();
  }

  public static abstract class UiProc<T> implements Proc<T>
  {
    @Override
    public final void invoke(final T param)
    {
      if (currentThreadIsUi())
        invokeUi(param);
      else
        UiThread.run(new Runnable()
        {
          @Override
          public void run()
          {
            invokeUi(param);
          }
        });
    }

    protected abstract void invokeUi(T param);
  }

  /**
   * Checks if we currently on UI thread. Throws IllegalStateException if called not from UI thread.
   */
  public static void checkUi()
  {
    if (!currentThreadIsUi())
      throw new IllegalStateException("Method should be called from UI thread.");
  }

  /**
   * Checks if we currently not on UI thread. Throws IllegalStateException if called from UI thread.
   */
  public static void checkNotUi()
  {
    if (currentThreadIsUi())
      throw new IllegalStateException("Method should NOT be called from UI thread.");
  }

  public static boolean currentThreadIsUi()
  {
    return sUiHandler.getLooper().getThread() == Thread.currentThread();
  }

  /**
   * Executes something on UI thread. If called from UI thread then given task will be executed synchronously.
   *
   * @param task the code that must be executed on UI thread.
   */
  public static void run(Runnable task)
  {
    if (currentThreadIsUi())
      task.run();
    else
      sUiHandler.post(task);
  }

  /**
   * Executes something on UI thread after last message queued in the application's main looper.
   *
   * @param task the code that must be executed later on UI thread.
   */
  public static void runLater(Runnable task)
  {
    runLater(task, 0);
  }

  /**
   * Executes something on UI thread after a given delay.
   *
   * @param task  the code that must be executed on UI thread after given delay.
   * @param delay The delay (in milliseconds) until the code will be executed.
   */
  public static void runLater(Runnable task, long delay)
  {
    sUiHandler.postDelayed(task, delay);
  }

  /**
   * Cancels execution of the given task that was previously queued with {@link #run(Runnable)},
   * {@link #runLater(Runnable)} or {@link #runLater(Runnable, long)} if it was not started yet.
   *
   * @param task the code that must be cancelled.
   */
  public static void cancelDelayedTasks(Runnable task)
  {
    sUiHandler.removeCallbacks(task);
  }
}
