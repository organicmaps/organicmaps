package app.organicmaps.downloader;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.Mockito.doCallRealMethod;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyNoInteractions;

import app.organicmaps.sdk.downloader.CountryItem;
import app.organicmaps.sdk.downloader.MapManager;
import java.lang.reflect.Field;
import org.junit.Test;

public class DownloaderServiceTest
{
  @Test
  public void countrySnapshotCombinesBothFlowsWithoutQueryingStorage() throws Exception
  {
    DownloaderNotifier notifier = mock(DownloaderNotifier.class);
    DownloaderService service = service(notifier);
    try (var countries = mockStatic(CountryItem.class))
    {
      service.onProgress("France_Nice", new MapManager.Progress(25, 100, 150, 200));

      verify(notifier).notifyProgress("France_Nice", 1000, 583);
      verify(service, never()).stopSelf(anyInt());
    }
  }

  @Test
  public void rootSnapshotDoesNotOverwriteCountryNotification() throws Exception
  {
    DownloaderNotifier notifier = mock(DownloaderNotifier.class);
    DownloaderService service = service(notifier);
    try (var countries = mockStatic(CountryItem.class))
    {
      countries.when(() -> CountryItem.isRoot("Countries")).thenReturn(true);
      service.onProgress("Countries", new MapManager.Progress(50, 100, 10, 30));

      verifyNoInteractions(notifier);
    }
  }

  @Test
  public void folderProgressKeepsOriginalCountryTarget() throws Exception
  {
    DownloaderNotifier notifier = mock(DownloaderNotifier.class);
    DownloaderService service = service(notifier);
    try (var countries = mockStatic(CountryItem.class))
    {
      service.onProgress("Slovenia", new MapManager.Progress(50, 100, 10, 20));

      verify(notifier).notifyProgress("Slovenia", 1000, 500);
    }
  }

  @Test
  public void terrainContinuesUsingSameNotificationAfterMapCompletion() throws Exception
  {
    DownloaderNotifier notifier = mock(DownloaderNotifier.class);
    DownloaderService service = service(notifier);
    try (var countries = mockStatic(CountryItem.class))
    {
      service.onProgress("France_Nice", new MapManager.Progress(0, 0, 150, 200));

      verify(notifier).notifyProgress("France_Nice", 1000, 750);
      verify(notifier, never()).cancelProgress();
      verify(notifier, never()).cancelError();
      verify(service, never()).stopSelf(anyInt());
    }
  }

  @Test
  public void idleProgressDoesNotDivideByZeroOrStopDuringValidation() throws Exception
  {
    DownloaderNotifier notifier = mock(DownloaderNotifier.class);
    DownloaderService service = service(notifier);
    try (var countries = mockStatic(CountryItem.class))
    {
      service.onProgress("France_Nice", new MapManager.Progress(0, 0, 0, 0));

      verifyNoInteractions(notifier);
      verify(service, never()).stopSelf(anyInt());
    }
  }

  @Test
  public void knownQueuedBytesStartDeterminateAtZero() throws Exception
  {
    DownloaderNotifier notifier = mock(DownloaderNotifier.class);
    DownloaderService service = service(notifier);
    try (var countries = mockStatic(CountryItem.class))
    {
      service.onProgress("France_Nice", new MapManager.Progress(0, 100, 0, 200));

      verify(notifier).notifyProgress("France_Nice", 1000, 0);
    }
  }

  @Test
  public void largeMapAndTerrainByteCountsRemainAccurate() throws Exception
  {
    DownloaderNotifier notifier = mock(DownloaderNotifier.class);
    DownloaderService service = service(notifier);
    try (var countries = mockStatic(CountryItem.class))
    {
      service.onProgress("France", new MapManager.Progress(1000000000L, 2000000000L, 1500000000L, 3000000000L));

      verify(notifier).notifyProgress("France", 1000, 500);
    }
  }

  private static DownloaderService service(DownloaderNotifier notifier) throws Exception
  {
    DownloaderService service = mock(DownloaderService.class);
    Field field = DownloaderService.class.getDeclaredField("mNotifier");
    field.setAccessible(true);
    field.set(service, notifier);
    doCallRealMethod().when(service).onProgress(anyString(), any(MapManager.Progress.class));
    return service;
  }
}
