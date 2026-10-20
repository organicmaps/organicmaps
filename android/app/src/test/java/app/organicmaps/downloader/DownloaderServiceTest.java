package app.organicmaps.downloader;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.ArgumentMatchers.anyList;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.Mockito.doCallRealMethod;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyNoInteractions;
import static org.mockito.Mockito.verifyNoMoreInteractions;

import app.organicmaps.sdk.downloader.CountryItem;
import app.organicmaps.sdk.downloader.MapManager;
import java.lang.reflect.Field;
import java.util.List;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.mockito.MockedStatic;

public class DownloaderServiceTest
{
  private MockedStatic<MapManager> mMaps;

  @Before
  public void setUp()
  {
    mMaps = mockStatic(MapManager.class);
    mMaps.when(() -> MapManager.isLeaf(anyString())).thenReturn(true);
    mMaps.when(() -> MapManager.isLeaf("Countries")).thenReturn(false);
    mMaps.when(() -> MapManager.isLeaf("Slovenia")).thenReturn(false);
  }

  @After
  public void tearDown()
  {
    mMaps.close();
  }

  @Test
  public void leafSnapshotCombinesBothFlows() throws Exception
  {
    DownloaderNotifier notifier = mock(DownloaderNotifier.class);
    DownloaderService service = service(notifier);
    service.onProgress("France_Nice", new MapManager.Progress(25, 100, 150, 200));

    verify(notifier).notifyProgress("France_Nice", 1000, 583);
    verify(service, never()).stopSelf(anyInt());
  }

  @Test
  public void rootSnapshotDoesNotOverwriteCountryNotification() throws Exception
  {
    DownloaderNotifier notifier = mock(DownloaderNotifier.class);
    DownloaderService service = service(notifier);
    service.onProgress("Countries", new MapManager.Progress(50, 100, 10, 30));

    verifyNoInteractions(notifier);
  }

  @Test
  public void folderProgressDoesNotOverwriteLeafNotification() throws Exception
  {
    DownloaderNotifier notifier = mock(DownloaderNotifier.class);
    DownloaderService service = service(notifier);
    service.onProgress("Slovenia", new MapManager.Progress(50, 100, 10, 20));

    verifyNoInteractions(notifier);
  }

  @Test
  public void hierarchyProgressPostsOnlyTheLeafNotification() throws Exception
  {
    DownloaderNotifier notifier = mock(DownloaderNotifier.class);
    DownloaderService service = service(notifier);
    service.onProgress("France_Nice", new MapManager.Progress(50, 100, 10, 20));
    service.onProgress("Slovenia", new MapManager.Progress(50, 200, 10, 40));
    service.onProgress("Countries", new MapManager.Progress(50, 300, 10, 60));

    verify(notifier).notifyProgress("France_Nice", 1000, 500);
    verifyNoMoreInteractions(notifier);
  }

  @Test
  public void terrainContinuesUsingSameNotificationAfterMapCompletion() throws Exception
  {
    DownloaderNotifier notifier = mock(DownloaderNotifier.class);
    DownloaderService service = service(notifier);
    service.onProgress("France_Nice", new MapManager.Progress(0, 0, 150, 200));

    verify(notifier).notifyProgress("France_Nice", 1000, 750);
    verify(notifier, never()).cancelProgress();
    verify(notifier, never()).cancelError();
    verify(service, never()).stopSelf(anyInt());
  }

  @Test
  public void idleProgressDoesNotDivideByZeroOrStopDuringValidation() throws Exception
  {
    DownloaderNotifier notifier = mock(DownloaderNotifier.class);
    DownloaderService service = service(notifier);
    service.onProgress("France_Nice", new MapManager.Progress(0, 0, 0, 0));

    verifyNoInteractions(notifier);
    verify(service, never()).stopSelf(anyInt());
  }

  @Test
  public void knownQueuedBytesStartDeterminateAtZero() throws Exception
  {
    DownloaderNotifier notifier = mock(DownloaderNotifier.class);
    DownloaderService service = service(notifier);
    service.onProgress("France_Nice", new MapManager.Progress(0, 100, 0, 200));

    verify(notifier).notifyProgress("France_Nice", 1000, 0);
  }

  @Test
  public void largeMapAndTerrainByteCountsRemainAccurate() throws Exception
  {
    DownloaderNotifier notifier = mock(DownloaderNotifier.class);
    DownloaderService service = service(notifier);
    service.onProgress("France", new MapManager.Progress(1000000000L, 2000000000L, 1500000000L, 3000000000L));

    verify(notifier).notifyProgress("France", 1000, 500);
  }

  @Test
  public void failureNotificationsUseTheRetryResultForTheFailedCountry() throws Exception
  {
    DownloaderNotifier notifier = mock(DownloaderNotifier.class);
    DownloaderService service = service(notifier);
    mMaps.when(() -> MapManager.isAutoRetryFailed("France_Nice")).thenReturn(false);
    mMaps.when(() -> MapManager.isAutoRetryFailed("France")).thenReturn(true);

    service.onStatusChanged(List.of(
        new MapManager.StorageCallbackData("France_Nice", CountryItem.STATUS_FAILED, CountryItem.ERROR_UNKNOWN, true),
        new MapManager.StorageCallbackData("France", CountryItem.STATUS_FAILED, CountryItem.ERROR_UNKNOWN, true)));

    verify(notifier).notifyDownloadFailed("France");
    verifyNoMoreInteractions(notifier);
  }

  @Test
  public void retryableAndHiddenFailuresDoNotPostTerminalNotifications() throws Exception
  {
    DownloaderNotifier notifier = mock(DownloaderNotifier.class);
    DownloaderService service = service(notifier);
    mMaps.when(() -> MapManager.isAutoRetryFailed("France_Nice")).thenReturn(false);

    service.onStatusChanged(List.of(
        new MapManager.StorageCallbackData("France_Nice", CountryItem.STATUS_FAILED, CountryItem.ERROR_UNKNOWN, true)));

    verifyNoInteractions(notifier);
  }

  @Test
  public void ancestorFailuresAndSuccessfulRowsDoNotQueryRetryState() throws Exception
  {
    DownloaderNotifier notifier = mock(DownloaderNotifier.class);
    DownloaderService service = service(notifier);

    service.onStatusChanged(List.of(
        new MapManager.StorageCallbackData("Slovenia", CountryItem.STATUS_FAILED, CountryItem.ERROR_UNKNOWN, false),
        new MapManager.StorageCallbackData("France_Nice", CountryItem.STATUS_DONE, CountryItem.ERROR_NONE, true)));

    mMaps.verify(() -> MapManager.isAutoRetryFailed(anyString()), never());
    verifyNoInteractions(notifier);
  }

  private static DownloaderService service(DownloaderNotifier notifier) throws Exception
  {
    DownloaderService service = mock(DownloaderService.class);
    Field field = DownloaderService.class.getDeclaredField("mNotifier");
    field.setAccessible(true);
    field.set(service, notifier);
    field = DownloaderService.class.getDeclaredField("mEnqueuingDownloads");
    field.setAccessible(true);
    field.setBoolean(service, true);
    doCallRealMethod().when(service).onStatusChanged(anyList());
    doCallRealMethod().when(service).onProgress(anyString(), any(MapManager.Progress.class));
    return service;
  }
}
