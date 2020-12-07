package com.mapswithme.util.log;

import android.app.ActivityManager;
import android.content.Context;
import android.os.Debug;
import android.os.Build;

import androidx.annotation.NonNull;
import com.mapswithme.maps.MwmApplication;

@SuppressWarnings("unused")
public class MemLogging
{
  public static String getMemoryInfo(@NonNull Context context)
  {
    final Debug.MemoryInfo debugMI = new Debug.MemoryInfo();
    Debug.getMemoryInfo(debugMI);
    final ActivityManager.MemoryInfo mi = new ActivityManager.MemoryInfo();
    final ActivityManager activityManager =
            (ActivityManager) context.getSystemService(Context.ACTIVITY_SERVICE);
    activityManager.getMemoryInfo(mi);

    StringBuilder log = new StringBuilder("Memory info: ");
    log.append(" Debug.getNativeHeapSize() = ").append(Debug.getNativeHeapSize() / 1024)
      .append("KB; Debug.getNativeHeapAllocatedSize() = ").append(Debug.getNativeHeapAllocatedSize() / 1024)
      .append("KB; Debug.getNativeHeapFreeSize() = ").append(Debug.getNativeHeapFreeSize() / 1024)
      .append("KB; debugMI.getTotalPrivateDirty() = ").append(debugMI.getTotalPrivateDirty())
      .append("KB; debugMI.getTotalPss() = ").append(debugMI.getTotalPss())
      .append("KB; mi.availMem = ").append(mi.availMem / 1024)
      .append("KB; mi.threshold = ").append(mi.threshold / 1024)
      .append("KB; mi.lowMemory = ").append(mi.lowMemory);
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN)
    {
      log.append(" mi.totalMem = ").append(mi.totalMem / 1024).append("KB;");
    }
    return log.toString();
  }
}
