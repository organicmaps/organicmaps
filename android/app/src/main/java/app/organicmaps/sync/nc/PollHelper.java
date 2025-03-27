package app.organicmaps.sync.nc;

import android.os.Handler;

import androidx.annotation.NonNull;

public class PollHelper implements Runnable
{
  private static final long DELAY_MS = 3000;
  @NonNull
  private final Handler mHandler;
  @NonNull
  private final PollAction pollAction;

  public PollHelper(@NonNull Handler handler, @NonNull PollAction pollAction)
  {
    this.mHandler = handler;
    this.pollAction = pollAction;
  }

  @Override
  public void run()
  {
    pollAction.poll();
    mHandler.postDelayed(this, DELAY_MS);
  }

  public void start()
  {
    mHandler.post(this);
  }

  public void stop()
  {
    mHandler.removeCallbacks(this);
  }

  public interface PollAction
  {
    void poll();
  }
}