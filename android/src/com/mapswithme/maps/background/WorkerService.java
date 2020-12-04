package com.mapswithme.maps.background;

import android.content.Context;
import android.content.Intent;
import androidx.annotation.NonNull;
import androidx.core.app.JobIntentService;
import android.text.TextUtils;
import android.util.Log;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.editor.Editor;
import com.mapswithme.maps.scheduling.JobIdMap;
import com.mapswithme.maps.ugc.UGC;
import com.mapswithme.util.CrashlyticsUtils;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

public class WorkerService extends JobIntentService
{
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);
  private static final String TAG = WorkerService.class.getSimpleName();
  private static final String ACTION_UPLOAD_OSM_CHANGES = "com.mapswithme.maps.action.upload_osm_changes";
  private static final String ACTION_UPLOAD_UGC = "com.mapswithme.maps.action.upload_ugc";

  /**
   * Starts this service to upload map edits to osm servers.
   */
  public static void startActionUploadOsmChanges(@NonNull Context context)
  {
    final Intent intent = new Intent(context, WorkerService.class);
    intent.setAction(WorkerService.ACTION_UPLOAD_OSM_CHANGES);
    JobIntentService.enqueueWork(context.getApplicationContext(), WorkerService.class,
                                 JobIdMap.getId(WorkerService.class), intent);
  }

  /**
   * Starts this service to upload UGC to our servers.
   */
  public static void startActionUploadUGC(@NonNull Context context)
  {
    final Intent intent = new Intent(context, WorkerService.class);
    intent.setAction(WorkerService.ACTION_UPLOAD_UGC);
    final int jobId = JobIdMap.getId(WorkerService.class);
    JobIntentService.enqueueWork(context, WorkerService.class, jobId, intent);
  }

  @Override
  protected void onHandleWork(@NonNull Intent intent)
  {
    final Context context = getApplicationContext();
    String msg = "onHandleIntent: " + intent + " app in background = "
                 + !MwmApplication.backgroundTracker(context).isForeground();
    LOGGER.i(TAG, msg);
    CrashlyticsUtils.INSTANCE.log(Log.INFO, TAG, msg);
    final String action = intent.getAction();

    if (TextUtils.isEmpty(action))
      return;

    if (!MwmApplication.from(context).arePlatformAndCoreInitialized())
      return;

    switch (action)
    {
      case ACTION_UPLOAD_OSM_CHANGES:
        handleActionUploadOsmChanges(context);
        break;

      case ACTION_UPLOAD_UGC:
        handleUploadUGC();
        break;
    }
  }

  private static void handleActionUploadOsmChanges(@NonNull Context context)
  {
    Editor.uploadChanges(context);
  }

  private static void handleUploadUGC()
  {
    UGC.nativeUploadUGC();
  }
}
