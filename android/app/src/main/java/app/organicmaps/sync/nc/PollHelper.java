package app.organicmaps.sync.nc;

import android.os.Handler;
import android.os.Looper;

import androidx.annotation.NonNull;
import androidx.lifecycle.Lifecycle;
import androidx.lifecycle.LifecycleEventObserver;

import java.util.Objects;


public class PollHelper implements Runnable
{
  private final long delayMs;
  @NonNull
  private final Handler mHandler;
  @NonNull
  private final PollAction mPollAction;

  private volatile boolean isRunning = false;   // volatile makes it okay to call start() and stop() from different threads (not currently needed though)
  private boolean noInitialDelay = false;       // whether to run pollAction immediately on start, or to delay as usual. Does not apply on the first run.
  private boolean isFirstRun = true;

  public PollHelper(long delayMs, @NonNull Handler handler, @NonNull PollAction pollAction)
  {
    this.delayMs = delayMs;
    this.mHandler = handler;
    this.mPollAction = pollAction;
  }

  @Override
  public void run()
  {
    if (!isRunning)
      return;
    mPollAction.poll();
    if (isRunning)
      mHandler.postDelayed(this, delayMs);
  }

  public void setNoInitialDelay(boolean noInitialDelay)
  {
    this.noInitialDelay = noInitialDelay;
  }

  public void start()
  {
    if (isRunning)
      return;
    isRunning = true;

    if (noInitialDelay || isFirstRun)
      mHandler.post(this);
    else
      mHandler.postDelayed(this, delayMs);

    isFirstRun = false;
  }

  public void stop()
  {
    isRunning = false;
    mHandler.removeCallbacks(this);
  }

  public interface PollAction
  {
    void poll();
  }
}