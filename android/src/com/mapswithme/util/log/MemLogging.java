package com.mapswithme.util.log;

import android.app.ActivityManager;
import android.content.Context;
import android.os.Debug;
import android.os.Build;
import com.mapswithme.maps.MWMApplication;
import java.text.DateFormat;
import java.util.Date;


public class MemLogging
{
  public static String GetMemoryInfo()
  {
    final Debug.MemoryInfo debugMI = new Debug.MemoryInfo();
    Debug.getMemoryInfo(debugMI);
    final ActivityManager.MemoryInfo mi = new ActivityManager.MemoryInfo();
    final Context content = MWMApplication.get().getApplicationContext();
    final ActivityManager activityManager =
            (ActivityManager) content.getSystemService(content.ACTIVITY_SERVICE);
    activityManager.getMemoryInfo(mi);
    StringBuilder log = new StringBuilder("Memory info: " + 
            " Debug.getNativeHeapSize() = " + Debug.getNativeHeapSize() / 1024 +
            "KB; Debug.getNativeHeapAllocatedSize() = " + Debug.getNativeHeapAllocatedSize() / 1024 +
            "KB; Debug.getNativeHeapFreeSize() = " + Debug.getNativeHeapFreeSize() / 1024 +
            "KB; debugMI.getTotalPrivateDirty() = " + debugMI.getTotalPrivateDirty() +
            "KB; debugMI.getTotalPss() = " + debugMI.getTotalPss() +
            "KB; mi.availMem = " + mi.availMem / 1024 +
            "KB; mi.threshold = " + mi.threshold / 1024 +
            "KB; mi.lowMemory = " + mi.lowMemory);
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN)
    {
      log.append(" mi.totalMem = " + mi.totalMem / 1024 + "KB.");
    }
    return log.toString();
  }
}
