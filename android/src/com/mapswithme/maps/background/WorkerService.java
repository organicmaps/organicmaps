package com.mapswithme.maps.background;

import android.app.IntentService;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.location.Location;
import android.location.LocationManager;
import android.net.Uri;
import android.text.TextUtils;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.MWMApplication;
import com.mapswithme.maps.R;
import com.mapswithme.util.LocationUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.StubLogger;
import com.mapswithme.util.statistics.Statistics;

import java.util.Timer;
import java.util.TimerTask;

/**
 * An {@link IntentService} subclass for handling asynchronous task requests in
 * a service on a separate handler thread.
 * <p/>
 */
public class WorkerService extends IntentService
{
  public static final String ACTION_PUSH_STATISTICS = "com.mapswithme.maps.action.stat";
  public static final String ACTION_CHECK_UPDATE = "com.mapswithme.maps.action.update";
  public static final String ACTION_DOWNLOAD_COUNTRY = "com.mapswithme.maps.action.download_country";
  public static final String ACTION_PROMO_NOTIFICATION_SHOW = "com.mapswithme.maps.action.notification.show";
  public static final String ACTION_PROMO_NOTIFICATION_CLICK = "com.mapswithme.maps.action.notification.click";

  private static final String PROMO_SHOW_EVENT_NAME = "PromoShowAndroid";
  private static final String PROMO_CLICK_EVENT_NAME = "PromoClickAndroid";

  private Logger mLogger = StubLogger.get();
  // = SimpleLogger.get("MWMWorkerService");


  /**
   * Starts this service to perform action  with the given parameters. If the
   * service is already performing a task this action will be queued.
   *
   * @see IntentService
   */
  public static void startActionPushStat(Context context)
  {
    Intent intent = new Intent(context, WorkerService.class);
    intent.setAction(ACTION_PUSH_STATISTICS);
    context.startService(intent);
  }

  /**
   * Starts this service to perform check update action with the given parameters. If the
   * service is already performing a task this action will be queued.
   *
   * @see IntentService
   */
  public static void startActionCheckUpdate(Context context)
  {
    Intent intent = new Intent(context, WorkerService.class);
    intent.setAction(ACTION_CHECK_UPDATE);
    context.startService(intent);
  }

  /**
   * Starts this service to perform check update action with the given parameters. If the
   * service is already performing a task this action will be queued.
   *
   * @see IntentService
   */
  public static void startActionDownload(Context context)
  {
    final Intent intent = new Intent(context, WorkerService.class);
    intent.setAction(WorkerService.ACTION_DOWNLOAD_COUNTRY);
    context.startService(intent);
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
      final String action = intent.getAction();

      switch (action)
      {
      case ACTION_CHECK_UPDATE:
        handleActionCheckUpdate();
        break;
      case ACTION_PUSH_STATISTICS:
        handleActionPushStat();
        break;
      case ACTION_DOWNLOAD_COUNTRY:
        handleActionCheckLocation();
        break;
      case ACTION_PROMO_NOTIFICATION_SHOW:
        showPromoNotification();
        break;
      case ACTION_PROMO_NOTIFICATION_CLICK:
        promoNotificationClicked();
        break;
      }
    }
  }

  private void handleActionCheckUpdate()
  {
    mLogger.d("Trying to update");
    if (!Framework.nativeIsDataVersionChanged()) return;

    final String countriesToUpdate = Framework.nativeGetOutdatedCountriesString();
    if (!TextUtils.isEmpty(countriesToUpdate))
    {
      mLogger.d("Update available! " + countriesToUpdate);
      Notifier.placeUpdateAvailable(countriesToUpdate);
    }
    // We are done with current version
    Framework.nativeUpdateSavedDataVersion();
    mLogger.d("Version updated");
  }

  private void handleActionPushStat()
  {
    // TODO: add server call here
  }

  private void handleActionCheckLocation()
  {
    final long delayMillis = 60000; // 60 seconds
    boolean isLocationValid = processLocation();
    Statistics.INSTANCE.trackWifiConnected(isLocationValid);
    if (!isLocationValid)
    {
      final Timer timer = new Timer();
      timer.schedule(new TimerTask()
      {
        @Override
        public void run()
        {
          Statistics.INSTANCE.trackWifiConnectedAfterDelay(processLocation(), delayMillis);
        }
      }, delayMillis);
    }
  }

  /**
   * Adds notification if current location isnt expired.
   *
   * @return whether notification was added
   */
  private boolean processLocation()
  {
    final LocationManager manager = (LocationManager) getApplication().getSystemService(Context.LOCATION_SERVICE);
    final Location l = manager.getLastKnownLocation(LocationManager.PASSIVE_PROVIDER);
    if (l != null && !LocationUtils.isExpired(l, l.getTime(), LocationUtils.LOCATION_EXPIRATION_TIME_MILLIS_LONG))
    {
      placeDownloadNotification(l);
      return true;
    }

    return false;
  }

  /**
   * Adds notification with download country suggest.
   *
   * @param l
   */
  private void placeDownloadNotification(Location l)
  {
    final String country = Framework.nativeGetCountryNameIfAbsent(l.getLatitude(), l.getLongitude());
    if (!TextUtils.isEmpty(country))
    {
      final SharedPreferences prefs = getApplicationContext().
          getSharedPreferences(getApplicationContext().getString(R.string.pref_file_name), Context.MODE_PRIVATE);
      final String lastNotification = prefs.getString(country, "");
      boolean shouldPlaceNotification = false; // should place notification only if it wasnt displayed for 180 days at least
      if (lastNotification.equals(""))
        shouldPlaceNotification = true;
      else
      {
        final long timeStamp = Long.valueOf(lastNotification);
        final long outdatedMillis = 180L * 24 * 60 * 60 * 1000; // half of year
        if (System.currentTimeMillis() - timeStamp > outdatedMillis)
          shouldPlaceNotification = true;
      }
      if (shouldPlaceNotification)
      {
        Notifier.placeDownloadSuggest(country, String.format(getApplicationContext().getString(R.string.download_location_country), country),
            Framework.nativeGetCountryIndex(l.getLatitude(), l.getLongitude()));
        prefs.edit().putString(country, String.valueOf(System.currentTimeMillis())).commit();
      }
    }
  }

  private void showPromoNotification()
  {
    Notifier.placePromoNotification();
    Statistics.INSTANCE.trackSimpleNamedEvent(PROMO_SHOW_EVENT_NAME);
  }

  private void promoNotificationClicked()
  {
    Intent intent = new Intent(Intent.ACTION_VIEW, Uri.parse(MWMApplication.get().getProVersionURL())).
        addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
    if (!Utils.isIntentAvailable(intent))
    {
      intent = new Intent(Intent.ACTION_VIEW, Uri.parse(MWMApplication.get().getDefaultProVersionURL())).
          addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
    }
    startActivity(intent);
    Statistics.INSTANCE.trackSimpleNamedEvent(PROMO_CLICK_EVENT_NAME);
  }
}
