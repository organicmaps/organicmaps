package app.organicmaps.editor;

import android.app.ForegroundServiceStartNotAllowedException;
import android.app.Notification;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.graphics.drawable.Drawable;
import android.os.Build;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.IBinder;
import android.os.Looper;
import android.os.Message;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.WorkerThread;
import androidx.appcompat.content.res.AppCompatResources;
import androidx.core.app.NotificationChannelCompat;
import androidx.core.app.NotificationCompat;
import androidx.core.app.NotificationManagerCompat;
import androidx.core.content.ContextCompat;

import app.organicmaps.R;
import app.organicmaps.util.Graphics;
import app.organicmaps.util.NetworkPolicy;
import app.organicmaps.util.log.Logger;

import java.util.Objects;

public class OsmUploadService extends Service
{
  private static final String TAG = OsmUploadService.class.getSimpleName();

  private static final String CHANNEL_ID = "OSM_UPLOAD";
  private static final int NOTIFICATION_ID = 12345679;

  private volatile Looper mServiceLooper;
  private volatile ServiceHandler mServiceHandler;

  private final class ServiceHandler extends Handler
  {
    public ServiceHandler(Looper looper)
    {
      super(looper);
    }

    @Override
    public void handleMessage(Message msg)
    {
      doInBackground();
      stopSelf(msg.arg1);
    }
  }

  private Notification createNotification()
  {
    final Drawable drawable = Objects.requireNonNull(AppCompatResources.getDrawable(this,
        R.drawable.ic_openstreetmap_color));
    return new NotificationCompat.Builder(this, CHANNEL_ID)
        .setCategory(NotificationCompat.CATEGORY_PROGRESS)
        .setPriority(Notification.PRIORITY_LOW)
        .setVisibility(NotificationCompat.VISIBILITY_PUBLIC)
        .setOngoing(true)
        .setShowWhen(false)
        .setOnlyAlertOnce(true)
        .setSmallIcon(R.drawable.ic_notification)
        .setColor(ContextCompat.getColor(this, R.color.notification))
        .setLargeIcon(Graphics.drawableToBitmap(drawable))
        .setContentTitle(getString(R.string.notification_osm_uploading))
        .build();
  }

  /**
   * Start a foreground service to upload OSM changes (if any).
   *
   * @param context Context to start service from.
   */
  public static void uploadOsmChanges(@NonNull Context context)
  {
    Logger.i(TAG);
    if (!Editor.nativeHasSomethingToUpload())
      return;

    if (!OsmOAuth.isAuthorized(context))
    {
      Logger.w(TAG, "Editor has changes, but OSM is not authorized");
      return;
    }

    if (!NetworkPolicy.getCurrentNetworkUsageStatus())
    {
      Logger.w(TAG, "Editor has changes, but network is not connected");
      return;
    }

    ContextCompat.startForegroundService(context, new Intent(context, OsmUploadService.class));
  }

  /**
   * Creates notification channel for navigation.
   *
   * @param context Context to create channel from.
   */
  public static void createNotificationChannel(@NonNull Context context)
  {
    Logger.i(TAG);

    final NotificationManagerCompat notificationManager = NotificationManagerCompat.from(context);
    final NotificationChannelCompat channel = new NotificationChannelCompat.Builder(CHANNEL_ID,
        NotificationManagerCompat.IMPORTANCE_LOW)
        .setName(context.getString(R.string.notification_channel_osm_uploader))
        .setLightsEnabled(false)    // less annoying
        .setVibrationEnabled(false) // less annoying
        .build();
    notificationManager.createNotificationChannel(channel);
  }

  @Override
  public void onCreate()
  {
    Logger.i(TAG);
    super.onCreate();

    final HandlerThread thread = new HandlerThread(TAG);
    thread.start();
    mServiceLooper = thread.getLooper();
    mServiceHandler = new ServiceHandler(mServiceLooper);
  }

  @Override
  public void onDestroy()
  {
    Logger.i(TAG);
    super.onDestroy();

    final NotificationManagerCompat notificationManager = NotificationManagerCompat.from(this);
    notificationManager.cancel(NOTIFICATION_ID);
    mServiceLooper.quit();
  }

  @Override
  public void onLowMemory()
  {
    super.onLowMemory();
    Logger.d(TAG, "onLowMemory()");
  }

  @Override
  public int onStartCommand(Intent intent, int flags, int startId)
  {
    Logger.i(TAG, "Starting foreground");
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S)
    {
      try
      {
        startForeground(NOTIFICATION_ID, createNotification());
      } catch (ForegroundServiceStartNotAllowedException e)
      {
        Logger.e(TAG, "Oops! ForegroundService is not allowed", e);
      }
    }
    else
    {
      startForeground(NOTIFICATION_ID, createNotification());
    }

    Message msg = mServiceHandler.obtainMessage();
    msg.arg1 = startId;
    msg.obj = intent;
    mServiceHandler.sendMessage(msg);

    return START_REDELIVER_INTENT;
  }

  @Nullable
  @Override
  public IBinder onBind(Intent intent)
  {
    Logger.i(TAG);
    return null;
  }

  @WorkerThread
  protected void doInBackground()
  {
    Editor.uploadChanges(this);
  }
}
