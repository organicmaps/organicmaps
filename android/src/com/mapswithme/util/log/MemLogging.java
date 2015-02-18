package com.mapswithme.util.log;

import android.app.ActivityManager;
import android.content.Context;
import android.os.Debug;
import android.os.Handler;
import android.util.Log;
import com.mapswithme.maps.BuildConfig;
import android.os.Build;


public class MemLogging
{
  private final static String TAG = "MemLogging";
  private Context mContext;
  private Handler mHandler;

  private Runnable logging = new Runnable()
  {
    @Override
    public void run()
    {
      final Debug.MemoryInfo debugMI = new Debug.MemoryInfo();
      Debug.getMemoryInfo(debugMI);
      final ActivityManager.MemoryInfo mi = new ActivityManager.MemoryInfo();
      final ActivityManager activityManager =
              (ActivityManager) mContext.getSystemService(mContext.ACTIVITY_SERVICE);
      activityManager.getMemoryInfo(mi);
      StringBuilder log = new StringBuilder("Memory info: Debug.getNativeHeapSize() = " + Debug.getNativeHeapSize() / 1024 +
              "KB; Debug.getNativeHeapAllocatedSize() = " + Debug.getNativeHeapAllocatedSize() / 1024 +
              "KB, Debug.getNativeHeapFreeSize() = " + Debug.getNativeHeapFreeSize() / 1024 +
              "KB. debugMI.getTotalPrivateDirty() = " + debugMI.getTotalPrivateDirty() +
              "KB. debugMI.getTotalPss() = " + debugMI.getTotalPss() +
              "KB. mi.availMem = " + mi.availMem / 1024 +
              "KB. mi.threshold = " + mi.threshold / 1024 +
              "KB. mi.lowMemory = " + mi.lowMemory + " ");
      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN)
      {
        log.append("mi.totalMem = " + mi.totalMem / 1024 + "KB.");
      }
      Log.d(TAG, log.toString());

      final long memLogPeriod = 3000;
      mHandler.postDelayed(logging, memLogPeriod);
    }
  };

  public MemLogging(Context context)
  {
    mContext = context;
    mHandler = new Handler();
  }

  public void startLogging()
  {
    if (BuildConfig.DEBUG)
      mHandler.post(logging);
  }

  public void stopLogging()
  {
    if (BuildConfig.DEBUG)
      mHandler.removeCallbacks(logging);
  }
}
