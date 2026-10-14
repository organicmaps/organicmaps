package app.organicmaps.downloader;

import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ServiceInfo;
import android.os.Build;
import android.os.IBinder;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.app.ServiceCompat;
import androidx.core.content.ContextCompat;
import app.organicmaps.sdk.downloader.CountryItem;
import app.organicmaps.sdk.downloader.MapManager;
import app.organicmaps.sdk.util.log.Logger;
import java.util.List;

public class DownloaderService extends Service implements MapManager.StorageCallback
{
  private static final String TAG = DownloaderService.class.getSimpleName();
  private static final String ACTION_CANCEL_DOWNLOAD = "ACTION_CANCEL_DOWNLOAD";
  private static final String ACTION_START_DOWNLOAD = "ACTION_START_DOWNLOAD";
  private static final String ACTION_RETRY_DOWNLOAD = "ACTION_RETRY_DOWNLOAD";
  private static final String ACTION_START_UPDATE = "ACTION_START_UPDATE";
  private static final String ACTION_START_TERRAIN_DOWNLOAD = "ACTION_START_TERRAIN_DOWNLOAD";
  private static final String EXTRA_COUNTRY_IDS = "EXTRA_COUNTRY_IDS";
  private static final String EXTRA_WITH_TERRAIN = "EXTRA_WITH_TERRAIN";

  private final DownloaderNotifier mNotifier = new DownloaderNotifier(this);
  private int mSubscriptionSlot;
  private boolean mEnqueuingDownloads;

  @Override
  public void onCreate()
  {
    super.onCreate();

    Logger.i(TAG);

    mSubscriptionSlot = MapManager.nativeSubscribe(this);
  }

  static PendingIntent buildCancelPendingIntent(Context context)
  {
    final int FLAG_IMMUTABLE = Build.VERSION.SDK_INT < Build.VERSION_CODES.M ? 0 : PendingIntent.FLAG_IMMUTABLE;
    Intent cancelIntent = new Intent(context, DownloaderService.class);
    cancelIntent.setAction(ACTION_CANCEL_DOWNLOAD);
    return PendingIntent.getService(context, 2, cancelIntent, PendingIntent.FLAG_UPDATE_CURRENT | FLAG_IMMUTABLE);
  }

  @Override
  public int onStartCommand(Intent intent, int flags, int startId)
  {
    final String action = intent != null ? intent.getAction() : null;

    // Cancel is sent via PendingIntent.getService(), so it has no foreground promotion deadline.
    if (ACTION_CANCEL_DOWNLOAD.equals(action))
    {
      cancelDownloads(startId);
      return START_NOT_STICKY;
    }

    // Download/retry/update actions are sent via startForegroundService().
    // Call startForeground() immediately to satisfy the contract, then enqueue work.
    // The download is deferred to here (instead of being started by MapManagerHelper) to
    // eliminate the race where a fast download completes before onStartCommand() runs.
    if (ACTION_START_DOWNLOAD.equals(action) || ACTION_RETRY_DOWNLOAD.equals(action)
        || ACTION_START_UPDATE.equals(action) || ACTION_START_TERRAIN_DOWNLOAD.equals(action))
    {
      mNotifier.cancelError();
      var notification = mNotifier.buildProgressNotification();
      Logger.i(TAG, "Starting Downloader Foreground Service");
      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q)
        ServiceCompat.startForeground(this, DownloaderNotifier.NOTIFICATION_ID, notification,
                                      ServiceInfo.FOREGROUND_SERVICE_TYPE_DATA_SYNC);
      else
        ServiceCompat.startForeground(this, DownloaderNotifier.NOTIFICATION_ID, notification, 0);

      // A no-op country can synchronously report an idle queue before the next country is enqueued.
      mEnqueuingDownloads = true;
      try
      {
        final String[] countryIds = intent.getStringArrayExtra(EXTRA_COUNTRY_IDS);
        if (countryIds != null)
        {
          if (ACTION_START_DOWNLOAD.equals(action))
          {
            MapManager.startDownload(countryIds, intent.getBooleanExtra(EXTRA_WITH_TERRAIN, true));
          }
          else if (ACTION_RETRY_DOWNLOAD.equals(action))
            MapManager.retryDownload(countryIds[0]);
          else if (ACTION_START_UPDATE.equals(action))
            MapManager.startUpdate(countryIds[0]);
          else
          {
            for (String countryId : countryIds)
              MapManager.nativeDownloadTerrain(countryId);
          }
        }
      }
      finally
      {
        mEnqueuingDownloads = false;
      }

      // The whole batch may still be a no-op, or may have failed synchronously.
      stopIfIdle(startId);
      return START_NOT_STICKY;
    }

    // No recognized action — stop if nothing to do.
    stopIfIdle(startId);
    return START_NOT_STICKY;
  }

  @Nullable
  @Override
  public IBinder onBind(Intent intent)
  {
    return null;
  }

  @Override
  public void onStatusChanged(List<MapManager.StorageCallbackData> data)
  {
    notifyDownloadFailures(data);
    if (!mEnqueuingDownloads)
      stopIfIdle(-1);
  }

  private void stopIfIdle(int startId)
  {
    if (MapManager.nativeIsDownloading())
      return;

    finishForeground();
    stopSelf(startId);
  }

  private void finishForeground()
  {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N)
      stopForeground(Service.STOP_FOREGROUND_REMOVE);
    else
      stopForeground(true);
    mNotifier.cancelProgress();
  }

  @Override
  public void onProgress(String countryId, MapManager.Progress progress)
  {
    if (CountryItem.isRoot(countryId))
      return;

    long downloaded = progress.mwmDownloaded + progress.twmDownloaded;
    long total = progress.mwmTotal + progress.twmTotal;
    if (total == 0)
      return;

    mNotifier.notifyProgress(countryId, 1000, (int) (1000.0 * downloaded / total));
  }

  private void cancelDownloads(int startId)
  {
    mEnqueuingDownloads = true;
    try
    {
      MapManager.nativeCancel(MapManager.nativeGetRoot());
      MapManager.nativeCancelTerrain(MapManager.nativeGetRoot());
    }
    finally
    {
      mEnqueuingDownloads = false;
    }
    finishForeground();
    mNotifier.cancelError();
    stopSelf(startId);
  }

  @Override
  public void onDestroy()
  {
    super.onDestroy();

    Logger.i(TAG, "onDestroy");

    MapManager.nativeUnsubscribe(mSubscriptionSlot);
    finishForeground();
  }

  @Override
  public void onTimeout(int startId)
  {
    onTimeout(startId, 0);
  }

  @Override
  public void onTimeout(int startId, int fgsType)
  {
    Logger.w(TAG, "Foreground service timed out, cancelling downloads and stopping the service"
                      + " startId: " + startId + " fgsType: " + fgsType);
    cancelDownloads(startId);
  }

  /**
   * Start the foreground service and enqueue downloads for the given countries.
   */
  static void startDownload(Context context, String... countryIds)
  {
    startDownload(context, countryIds, true);
  }

  static void startDownload(Context context, @NonNull String[] countryIds, boolean withTerrain)
  {
    Logger.i(TAG);
    Intent intent = new Intent(context, DownloaderService.class);
    intent.setAction(ACTION_START_DOWNLOAD);
    intent.putExtra(EXTRA_COUNTRY_IDS, countryIds);
    intent.putExtra(EXTRA_WITH_TERRAIN, withTerrain);
    ContextCompat.startForegroundService(context, intent);
  }

  /**
   * Start the foreground service and retry a failed download.
   */
  static void startRetryDownload(Context context, @NonNull String countryId)
  {
    Logger.i(TAG);
    Intent intent = new Intent(context, DownloaderService.class);
    intent.setAction(ACTION_RETRY_DOWNLOAD);
    intent.putExtra(EXTRA_COUNTRY_IDS, new String[] {countryId});
    ContextCompat.startForegroundService(context, intent);
  }

  /**
   * Start the foreground service and enqueue an update for the given root.
   */
  static void startUpdate(Context context, @NonNull String root)
  {
    Logger.i(TAG);
    Intent intent = new Intent(context, DownloaderService.class);
    intent.setAction(ACTION_START_UPDATE);
    intent.putExtra(EXTRA_COUNTRY_IDS, new String[] {root});
    ContextCompat.startForegroundService(context, intent);
  }

  static void startTerrainDownload(Context context, String... countryIds)
  {
    Intent intent = new Intent(context, DownloaderService.class);
    intent.setAction(ACTION_START_TERRAIN_DOWNLOAD);
    intent.putExtra(EXTRA_COUNTRY_IDS, countryIds);
    ContextCompat.startForegroundService(context, intent);
  }

  private void notifyDownloadFailures(List<MapManager.StorageCallbackData> data)
  {
    for (MapManager.StorageCallbackData item : data)
    {
      if (!item.isLeafNode)
        continue;
      boolean mapFailed = item.newStatus == CountryItem.STATUS_FAILED && MapManager.nativeIsAutoretryFailed();
      boolean terrainFailed = CountryItem.fill(item.countryId).terrainStatus == CountryItem.TERRAIN_FAILED
                           && MapManager.nativeIsTerrainAutoretryFailed();
      if (mapFailed || terrainFailed)
      {
        mNotifier.notifyDownloadFailed(item.countryId);
        return;
      }
    }
  }
}
