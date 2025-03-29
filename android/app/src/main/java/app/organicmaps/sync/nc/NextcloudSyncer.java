package app.organicmaps.sync.nc;

import android.content.Context;
import android.content.SharedPreferences;
import android.os.Handler;
import android.os.HandlerThread;

import androidx.annotation.NonNull;

import app.organicmaps.R;

public enum NextcloudSyncer
{
  INSTANCE;

  public static final long SYNC_DELAY_MS = 10_000;

  final PollHelper mPollHelper;

  private NextcloudSyncer()
  {
    HandlerThread handlerThread = new HandlerThread("NcSyncerThread");
    handlerThread.start();
    mPollHelper = new PollHelper(SYNC_DELAY_MS, new Handler(handlerThread.getLooper()), this::performSync);
  }

  private void performSync()
  {

  }

    @SuppressWarnings("NotNullFieldNotInitialized")
    @NonNull
    private SharedPreferences prefs;

    public void initialize(@NonNull Context context)
    {
      prefs = context.getSharedPreferences(context.getString(R.string.pref_nc_sync_filename),Context.MODE_PRIVATE);
    }

  // should be called after framework is initialized. Which is the case when calling it inside MwmApplication.onForeground
  public void resumeSync()
  {
    mPollHelper.start();
  }

  public void pauseSync()
  {
    mPollHelper.stop();
  }
}
