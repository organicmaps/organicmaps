package com.mapswithme.maps.background;

import android.app.IntentService;
import android.content.Intent;
import android.content.SharedPreferences;
import android.text.TextUtils;
import android.util.Log;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.editor.Editor;
import com.mapswithme.maps.ugc.UGC;
import com.mapswithme.util.CrashlyticsUtils;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

public class WorkerService extends IntentService
{
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);
  private static final String TAG = WorkerService.class.getSimpleName();
  private static final String ACTION_UPLOAD_OSM_CHANGES = "com.mapswithme.maps.action.upload_osm_changes";
  private static final String ACTION_UPLOAD_UGC = "com.mapswithme.maps.action.upload_ugc";

  private final boolean mArePlatformAndCoreInitialized =
      MwmApplication.get().arePlatformAndCoreInitialized();

  /**
   * Starts this service to upload map edits to osm servers.
   */
  public static void startActionUploadOsmChanges()
  {
    final Intent intent = new Intent(MwmApplication.get(), WorkerService.class);
    intent.setAction(WorkerService.ACTION_UPLOAD_OSM_CHANGES);
    MwmApplication.get().startService(intent);
  }

  /**
   * Starts this service to upload UGC to our servers.
   */
  public static void startActionUploadUGC()
  {
    final Intent intent = new Intent(MwmApplication.get(), WorkerService.class);
    intent.setAction(WorkerService.ACTION_UPLOAD_UGC);
    MwmApplication.get().startService(intent);
  }

  public WorkerService()
  {
    super("WorkerService");
  }

  @Override
  protected void onHandleIntent(Intent intent)
  {
    if (intent == null)
      return;

    String msg = "onHandleIntent: " + intent + " app in background = "
                 + !MwmApplication.backgroundTracker().isForeground();
    LOGGER.i(TAG, msg);
    CrashlyticsUtils.log(Log.INFO, TAG, msg);
    final String action = intent.getAction();

    if (TextUtils.isEmpty(action))
      return;

    if (!mArePlatformAndCoreInitialized)
      return;

    switch (action)
    {
    case ACTION_UPLOAD_OSM_CHANGES:
      handleActionUploadOsmChanges();
      break;

    case ACTION_UPLOAD_UGC:
      handleUploadUGC();
      break;
    }
  }

  private static void handleActionUploadOsmChanges()
  {
    Editor.uploadChanges();
  }

  private static void handleUploadUGC()
  {
    UGC.nativeUploadUGC();
  }
}
