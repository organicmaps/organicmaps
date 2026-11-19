package app.organicmaps.contacts;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Intent;
import android.content.pm.ServiceInfo;
import android.os.Build;
import android.os.IBinder;
import androidx.core.app.NotificationCompat;
import androidx.core.app.ServiceCompat;
import androidx.lifecycle.Observer;
import app.organicmaps.R;
import app.organicmaps.bookmarks.BookmarkCategoriesActivity;

/** User-started, non-sticky offline import. Never restarts contact access after process death. */
public final class ContactsImportService extends Service
{
  private static final String CHANNEL = "contact_import";
  private static final int NOTIFICATION = 7341;
  private ContactsImportCoordinator coordinator;
  private long lastProgressUpdate;
  private final Observer<ContactsImporter.State> observer = this::update;

  @Override
  public void onCreate()
  {
    super.onCreate();
    coordinator = ContactsImportCoordinator.getInstance(getApplication());
    if (Build.VERSION.SDK_INT >= 26)
      getSystemService(NotificationManager.class)
          .createNotificationChannel(new NotificationChannel(CHANNEL, getString(R.string.bookmarks_import_contacts),
                                                             NotificationManager.IMPORTANCE_LOW));
  }

  private Notification notification(ContactsImporter.State state)
  {
    int flags = PendingIntent.FLAG_UPDATE_CURRENT | PendingIntent.FLAG_IMMUTABLE;
    PendingIntent open = PendingIntent.getActivity(this, 0, new Intent(this, BookmarkCategoriesActivity.class), flags);
    PendingIntent cancel =
        PendingIntent.getService(this, 1, new Intent(this, ContactsImportService.class).setAction("cancel"), flags);
    String message = getString(R.string.bookmarks_contacts_import_reading);
    if (state.phase == ContactsImporter.Phase.RESOLVING)
      message = getString(R.string.bookmarks_contacts_import_resolving, state.processed, state.total);
    else if (state.phase == ContactsImporter.Phase.SAVING)
      message = getString(R.string.bookmarks_contacts_import_saving);
    return new NotificationCompat.Builder(this, CHANNEL)
        .setSmallIcon(app.organicmaps.branding.R.drawable.ic_splash)
        .setContentTitle(getString(R.string.bookmarks_import_contacts))
        .setContentText(message)
        .setContentIntent(open)
        .setOnlyAlertOnce(true)
        .setOngoing(true)
        .setProgress(state.total, state.processed, state.total == 0)
        .addAction(0, getString(R.string.cancel), cancel)
        .build();
  }

  @Override
  public int onStartCommand(Intent intent, int flags, int startId)
  {
    if (intent == null || "cancel".equals(intent.getAction()))
    {
      coordinator.cancel();
      stopSelf();
      return START_NOT_STICKY;
    }
    ServiceCompat.startForeground(this, NOTIFICATION, notification(coordinator.getState().getValue()),
                                  Build.VERSION.SDK_INT >= 29 ? ServiceInfo.FOREGROUND_SERVICE_TYPE_DATA_SYNC : 0);
    coordinator.begin();
    coordinator.getState().observeForever(observer);
    return START_NOT_STICKY;
  }

  private void update(ContactsImporter.State state)
  {
    if (!state.isRunning())
    {
      stopSelf();
      return;
    }
    long now = android.os.SystemClock.elapsedRealtime();
    if (now - lastProgressUpdate < 500)
      return;
    lastProgressUpdate = now;
    if (Build.VERSION.SDK_INT < 33
        || checkSelfPermission(android.Manifest.permission.POST_NOTIFICATIONS)
               == android.content.pm.PackageManager.PERMISSION_GRANTED)
      androidx.core.content.ContextCompat.getSystemService(this, NotificationManager.class)
          .notify(NOTIFICATION, notification(state));
  }

  @Override
  public void onTimeout(int startId)
  {
    onTimeout(startId, 0);
  }
  @Override
  public void onTimeout(int startId, int type)
  {
    coordinator.cancel();
    stopSelf();
  }
  @Override
  public void onDestroy()
  {
    coordinator.getState().removeObserver(observer);
    coordinator.cancel();
    ServiceCompat.stopForeground(this, ServiceCompat.STOP_FOREGROUND_REMOVE);
    super.onDestroy();
  }
  @Override
  public IBinder onBind(Intent intent)
  {
    return null;
  }
}
