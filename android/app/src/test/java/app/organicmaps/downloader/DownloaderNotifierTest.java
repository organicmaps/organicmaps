package app.organicmaps.downloader;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.RETURNS_SELF;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockConstruction;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;
import static org.mockito.Mockito.withSettings;

import android.app.Notification;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import androidx.core.app.NotificationCompat;
import androidx.core.app.NotificationManagerCompat;
import androidx.core.content.ContextCompat;
import app.organicmaps.R;
import app.organicmaps.sdk.util.StringUtils;
import app.organicmaps.sdk.util.log.Logger;
import org.junit.Test;
import org.mockito.MockedConstruction;
import org.mockito.MockedStatic;

public class DownloaderNotifierTest
{
  @Test
  public void progressUsesOneNotificationWithOriginalAppTitleAndCancelAction()
  {
    try (var mocks = new NotificationMocks())
    {
      DownloaderNotifier notifier = new DownloaderNotifier(mocks.context);
      notifier.notifyProgress(null, 1000, 250);
      notifier.notifyProgress(null, 1000, 750);

      var builder = mocks.builders.constructed().get(0);
      verify(builder).setContentTitle("Organic Maps");
      verify(builder).setContentText("Downloading: ");
      verify(builder).setProgress(1000, 250, false);
      verify(builder).setProgress(1000, 750, false);
      verify(mocks.manager, times(2)).notify(DownloaderNotifier.NOTIFICATION_ID, builder.build());
      mocks.pendingIntents.verify(
          () -> PendingIntent.getActivity(eq(mocks.context), eq(0), any(Intent.class), anyInt()));
      mocks.pendingIntents.verify(
          () -> PendingIntent.getService(eq(mocks.context), eq(2), any(Intent.class), anyInt()));
    }
  }

  @Test
  public void initialProgressIsIndeterminateUntilKnownBytesArrive()
  {
    try (var mocks = new NotificationMocks())
    {
      DownloaderNotifier notifier = new DownloaderNotifier(mocks.context);
      notifier.buildProgressNotification();
      notifier.notifyProgress(null, 1000, 0);

      var builder = mocks.builders.constructed().get(0);
      verify(builder).setProgress(0, 0, true);
      verify(builder).setProgress(1000, 0, false);
    }
  }

  @Test
  public void anotherForegroundStartPreservesDeterminateProgress()
  {
    try (var mocks = new NotificationMocks())
    {
      DownloaderNotifier notifier = new DownloaderNotifier(mocks.context);
      notifier.notifyProgress(null, 1000, 750);
      notifier.buildProgressNotification();

      var builder = mocks.builders.constructed().get(0);
      verify(builder, times(1)).setProgress(1000, 750, false);
      verify(builder, never()).setProgress(0, 0, true);
    }
  }

  @Test
  public void stoppingForegroundProgressPreservesIndependentErrorNotification()
  {
    try (var mocks = new NotificationMocks())
    {
      DownloaderNotifier notifier = new DownloaderNotifier(mocks.context);
      notifier.notifyProgress(null, 1000, 750);
      notifier.notifyDownloadFailed(null);
      notifier.cancelProgress();

      var errorBuilder = mocks.builders.constructed().get(1);
      verify(errorBuilder).setContentTitle("Organic Maps");
      verify(errorBuilder).setContentText("Download failed");
      verify(mocks.manager).notify("downloader_error", DownloaderNotifier.NOTIFICATION_ID, errorBuilder.build());
      mocks.pendingIntents.verify(
          () -> PendingIntent.getActivity(eq(mocks.context), eq(0), any(Intent.class), anyInt()));
      mocks.pendingIntents.verify(
          () -> PendingIntent.getActivity(eq(mocks.context), eq(1), any(Intent.class), anyInt()));
      verify(mocks.manager).cancel(DownloaderNotifier.NOTIFICATION_ID);
      verify(mocks.manager, never()).cancel("downloader_error", DownloaderNotifier.NOTIFICATION_ID);
      notifier.notifyProgress(null, 1000, 800);
      verify(mocks.builders.constructed().get(2)).setProgress(1000, 800, false);
      verify(mocks.manager, never()).cancel("downloader_error", DownloaderNotifier.NOTIFICATION_ID);
    }
  }

  @Test
  public void explicitErrorCancellationDoesNotCancelActiveProgress()
  {
    try (var mocks = new NotificationMocks())
    {
      DownloaderNotifier notifier = new DownloaderNotifier(mocks.context);
      notifier.notifyProgress(null, 1000, 250);
      notifier.notifyDownloadFailed(null);
      notifier.cancelError();

      verify(mocks.manager).cancel("downloader_error", DownloaderNotifier.NOTIFICATION_ID);
      verify(mocks.manager, never()).cancel(DownloaderNotifier.NOTIFICATION_ID);
    }
  }

  private static final class NotificationMocks implements AutoCloseable
  {
    final Context context = mock(Context.class);
    final NotificationManagerCompat manager = mock(NotificationManagerCompat.class);
    final MockedStatic<NotificationManagerCompat> managers = mockStatic(NotificationManagerCompat.class);
    final MockedStatic<ContextCompat> contextCompat = mockStatic(ContextCompat.class);
    final MockedStatic<PendingIntent> pendingIntents = mockStatic(PendingIntent.class);
    final MockedStatic<StringUtils> stringUtils = mockStatic(StringUtils.class);
    final MockedStatic<Logger> logger = mockStatic(Logger.class);
    final MockedConstruction<Intent> intents =
        mockConstruction(Intent.class, withSettings().defaultAnswer(RETURNS_SELF));
    final MockedConstruction<NotificationCompat.Builder> builders =
        mockConstruction(NotificationCompat.Builder.class, withSettings().defaultAnswer(RETURNS_SELF),
                         (builder, unused) -> when(builder.build()).thenReturn(mock(Notification.class)));

    NotificationMocks()
    {
      managers.when(() -> NotificationManagerCompat.from(context)).thenReturn(manager);
      when(context.getString(app.organicmaps.branding.R.string.app_name)).thenReturn("Organic Maps");
      when(context.getString(R.string.downloader_downloading)).thenReturn("Downloading:");
      when(context.getString(R.string.download_country_failed, "")).thenReturn("Download failed");
      when(context.getString(R.string.notification_ticker_ltr, "Organic Maps", "Download failed"))
          .thenReturn("Download failed");
    }

    @Override
    public void close()
    {
      builders.close();
      intents.close();
      logger.close();
      stringUtils.close();
      pendingIntents.close();
      contextCompat.close();
      managers.close();
    }
  }
}
