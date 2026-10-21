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
  private MockedStatic<CountryItem> mItems;

  @Before
  public void setUp()
  {
    mMaps = mockStatic(MapManager.class);
    mMaps.when(() -> MapManager.isLeaf(anyString())).thenReturn(true);
    mMaps.when(() -> MapManager.isLeaf("Countries")).thenReturn(false);
    mMaps.when(() -> MapManager.isLeaf("Slovenia")).thenReturn(false);
    mItems = mockStatic(CountryItem.class);
    mItems.when(() -> CountryItem.isRoot("Countries")).thenReturn(true);
  }

  @After
  public void tearDown()
  {
    mItems.close();
    mMaps.close();
  }

  @Test
  public void leafSnapshotCombinesBothFlows() throws Exception
  {
    DownloaderNotifier notifier = mock(DownloaderNotifier.class);
    DownloaderService service = service(notifier);
    service.onProgress("France_Nice", new MapManager.Progress(25, 100, 150, 200));

    verify(notifier).notifyProgress("France_Nice", 100, 58);
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

    verify(notifier).notifyProgress("France_Nice", 100, 50);
    verifyNoMoreInteractions(notifier);
  }

  @Test
  public void terrainContinuesUsingSameNotificationAfterMapCompletion() throws Exception
  {
    DownloaderNotifier notifier = mock(DownloaderNotifier.class);
    DownloaderService service = service(notifier);
    service.onProgress("France_Nice", new MapManager.Progress(0, 0, 150, 200));

    verify(notifier).notifyProgress("France_Nice", 100, 75);
    verify(notifier, never()).cancelProgress();
    verify(notifier, never()).cancelError();
  }

  @Test
  public void sharedTerrainBlockNotifiesOncePerBatch() throws Exception
  {
    DownloaderNotifier notifier = mock(DownloaderNotifier.class);
    DownloaderService service = service(notifier);
    // A shared block: every owner, then the root.
    service.onProgress("France_Nice", new MapManager.Progress(0, 0, 10, 100));
    service.onProgress("Monaco", new MapManager.Progress(0, 0, 10, 100));
    service.onProgress("Countries", new MapManager.Progress(0, 0, 10, 100));
    // The next block is owned only by Monaco.
    service.onProgress("Monaco", new MapManager.Progress(0, 0, 25, 100));
    service.onProgress("Countries", new MapManager.Progress(0, 0, 25, 100));

    verify(notifier).notifyProgress("France_Nice", 100, 10);
    verify(notifier).notifyProgress("Monaco", 100, 25);
    verifyNoMoreInteractions(notifier);
  }

  @Test
  public void idleProgressDoesNotDivideByZeroOrStopDuringValidation() throws Exception
  {
    DownloaderNotifier notifier = mock(DownloaderNotifier.class);
    DownloaderService service = service(notifier);
    service.onProgress("France_Nice", new MapManager.Progress(0, 0, 0, 0));

    verifyNoInteractions(notifier);
  }

  @Test
  public void knownQueuedBytesStartDeterminateAtZero() throws Exception
  {
    DownloaderNotifier notifier = mock(DownloaderNotifier.class);
    DownloaderService service = service(notifier);
    service.onProgress("France_Nice", new MapManager.Progress(0, 100, 0, 200));

    verify(notifier).notifyProgress("France_Nice", 100, 0);
  }

  @Test
  public void largeMapAndTerrainByteCountsRemainAccurate() throws Exception
  {
    DownloaderNotifier notifier = mock(DownloaderNotifier.class);
    DownloaderService service = service(notifier);
    service.onProgress("France", new MapManager.Progress(1000000000L, 2000000000L, 1500000000L, 3000000000L));

    verify(notifier).notifyProgress("France", 100, 50);
  }

  @Test
  public void exhaustedRetryNotifiesTheFirstFailedLeaf() throws Exception
  {
    DownloaderNotifier notifier = mock(DownloaderNotifier.class);
    DownloaderService service = service(notifier);
    mMaps.when(MapManager::isAutoRetryFailed).thenReturn(true);

    service.onStatusChanged(List.of(
        new MapManager.StorageCallbackData("France_Nice", CountryItem.STATUS_FAILED, CountryItem.ERROR_UNKNOWN, true),
        new MapManager.StorageCallbackData("France", CountryItem.STATUS_FAILED, CountryItem.ERROR_UNKNOWN, true)));

    verify(notifier).notifyDownloadFailed("France_Nice");
    verifyNoMoreInteractions(notifier);
  }

  @Test
  public void retryableAndHiddenFailuresDoNotPostTerminalNotifications() throws Exception
  {
    DownloaderNotifier notifier = mock(DownloaderNotifier.class);
    DownloaderService service = service(notifier);
    mMaps.when(MapManager::isAutoRetryFailed).thenReturn(false);

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

    mMaps.verify(MapManager::isAutoRetryFailed, never());
    verifyNoInteractions(notifier);
  }

  @Test
  public void progressPostsOnlyPercentChanges() throws Exception
  {
    DownloaderNotifier notifier = mock(DownloaderNotifier.class);
    DownloaderService service = service(notifier);
    // Every Core batch ends with the root.
    for (long downloaded : new long[] {10, 15, 20})
    {
      service.onProgress("France_Nice", new MapManager.Progress(downloaded, 1000, 0, 0));
      service.onProgress("Countries", new MapManager.Progress(downloaded, 1000, 0, 0));
    }

    verify(notifier).notifyProgress("France_Nice", 100, 1);
    verify(notifier).notifyProgress("France_Nice", 100, 2);
    verifyNoMoreInteractions(notifier);
  }

  @Test
  public void idleStatusStopsTheServiceUnlessEnqueuingOrDownloading() throws Exception
  {
    DownloaderNotifier notifier = mock(DownloaderNotifier.class);
    DownloaderService service = service(notifier);
    service.onStatusChanged(List.of());
    verify(service, never()).stopSelf(anyInt());

    Field enqueuing = DownloaderService.class.getDeclaredField("mEnqueuingDownloads");
    enqueuing.setAccessible(true);
    enqueuing.setBoolean(service, false);
    mMaps.when(MapManager::isDownloading).thenReturn(true);
    service.onStatusChanged(List.of());
    verify(service, never()).stopSelf(anyInt());

    mMaps.when(MapManager::isDownloading).thenReturn(false);
    service.onStatusChanged(List.of());
    verify(service).stopSelf(-1);
    verify(notifier).cancelProgress();
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
