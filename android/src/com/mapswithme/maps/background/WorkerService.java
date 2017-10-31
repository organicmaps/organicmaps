package com.mapswithme.maps.background;

import android.app.IntentService;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.location.Location;
import android.text.TextUtils;
import android.util.Log;

import com.crashlytics.android.Crashlytics;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.downloader.CountryItem;
import com.mapswithme.maps.downloader.MapManager;
import com.mapswithme.maps.editor.Editor;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.maps.ugc.UGC;
import com.mapswithme.util.CrashlyticsUtils;
import com.mapswithme.util.PermissionsUtils;
import com.mapswithme.util.concurrency.UiThread;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

import java.util.concurrent.CountDownLatch;

public class WorkerService extends IntentService
{
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);
  private static final String TAG = WorkerService.class.getSimpleName();
  private static final String ACTION_CHECK_LOCATIION = "com.mapswithme.maps.action.check_location";
  private static final String ACTION_UPLOAD_OSM_CHANGES = "com.mapswithme.maps.action.upload_osm_changes";
  private static final String ACTION_UPLOAD_UGC = "com.mapswithme.maps.action.upload_ugc";

  private static final SharedPreferences PREFS = MwmApplication.prefs();

  /**
   * Starts this service to check if map download for current location is available. If the
   * service is already performing a task this action will be queued.
   */
  static void startActionCheckLocation(Context context)
  {
    final Intent intent = new Intent(context, WorkerService.class);
    intent.setAction(WorkerService.ACTION_CHECK_LOCATIION);
    context.startService(intent);
  }

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
    if (intent != null)
    {
      String msg = "onHandleIntent: " + intent + " app in background = "
                   + !MwmApplication.backgroundTracker().isForeground();
      LOGGER.i(TAG, msg);
      CrashlyticsUtils.log(Log.INFO, TAG, msg);
      final String action = intent.getAction();
      switch (action)
      {
      case ACTION_CHECK_LOCATIION:
        handleActionCheckLocation();
        break;

      case ACTION_UPLOAD_OSM_CHANGES:
        handleActionUploadOsmChanges();
        break;

      case ACTION_UPLOAD_UGC:
        handleUploadUGC();
        break;
      }
    }
  }

  private static void handleActionCheckLocation()
  {
    if (!checkLocationDelayed(0))
      checkLocationDelayed(60000);  // 1 minute delay
  }

  private static boolean checkLocationDelayed(long delay)
  {
    class Holder
    {
      final CountDownLatch hook = new CountDownLatch(1);
      boolean result;
    }

    final Holder holder = new Holder();

    UiThread.runLater(new Runnable()
    {
      @Override
      public void run()
      {
        holder.result = processLocation();

        // Release awaiting caller
        holder.hook.countDown();
      }
    }, delay);

    try
    {
      holder.hook.await();
    }
    catch (InterruptedException ignored) {}


    return holder.result;
  }

  private static void handleActionUploadOsmChanges()
  {
    Editor.uploadChanges();
  }

  private static void handleUploadUGC()
  {
    UGC.nativeUploadUGC();
  }

  @android.support.annotation.UiThread
  private static boolean processLocation()
  {
    if (!PermissionsUtils.isExternalStorageGranted())
      return false;

    MwmApplication application = MwmApplication.get();
    if (!application.arePlatformAndCoreInitialized())
    {
      boolean success = application.initPlatformAndCore();
      if (!success)
      {
        String message = "Native part couldn't be initialized successfully";
        LOGGER.e(TAG, message);
        CrashlyticsUtils.log(Log.ERROR, TAG, message);
        return false;
      }
    }

    Location l = LocationHelper.INSTANCE.getLastKnownLocation();
    if (l == null)
      return false;

    String country = MapManager.nativeFindCountry(l.getLatitude(), l.getLongitude());
    if (TextUtils.isEmpty(country))
      return false;

    final String lastNotification = PREFS.getString(country, null);
    if (lastNotification != null)
    {
      // Do not place notification if it was displayed less than 180 days ago.
      final long timeStamp = Long.valueOf(lastNotification);
      final long outdatedMillis = 180L * 24 * 60 * 60 * 1000;
      if (System.currentTimeMillis() - timeStamp < outdatedMillis)
        return true;
    }

    if (MapManager.nativeGetStatus(country) != CountryItem.STATUS_DOWNLOADABLE)
      return true;

    String name = MapManager.nativeGetName(country);
    Notifier.notifyDownloadSuggest(name, MwmApplication.get().getString(R.string.download_location_country, name), country);
    PREFS.edit().putString(country, String.valueOf(System.currentTimeMillis())).apply();
    return true;
  }
}
