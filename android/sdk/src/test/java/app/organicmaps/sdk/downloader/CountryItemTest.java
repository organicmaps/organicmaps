package app.organicmaps.sdk.downloader;

import static org.junit.Assert.assertEquals;

import org.junit.Test;

public class CountryItemTest
{
  @Test
  public void installedEntriesShowCombinedLocalSize()
  {
    CountryItem country = new CountryItem("France");
    country.size = 130;
    country.totalSize = 500;
    country.downloadSize = 40;
    country.status = CountryItem.STATUS_DONE;
    assertEquals(130, country.getDisplaySize(true));
    assertEquals(130, country.getDisplaySize(false));
  }

  @Test
  public void partlyDownloadedFolderShowsOnlyInstalledRegionsInMyMaps()
  {
    CountryItem folder = new CountryItem("Group");
    folder.status = CountryItem.STATUS_PARTLY;
    folder.childCount = 2;
    folder.totalChildCount = 3;
    folder.size = 330; // Two installed MWMs and their shared TWM, counted once.
    folder.totalSize = 640;
    folder.downloadSize = 300;
    assertEquals(330, folder.getDisplaySize(true));
    assertEquals(300, folder.getDisplaySize(false));
  }

  @Test
  public void pendingEntriesShowTransferSizeInsteadOfInstalledOrFullSize()
  {
    CountryItem country = new CountryItem("France");
    country.size = 130;
    country.totalSize = 500;
    country.downloadSize = 40;
    for (int status :
         new int[] {CountryItem.STATUS_UPDATABLE, CountryItem.STATUS_DOWNLOADABLE, CountryItem.STATUS_FAILED})
    {
      country.status = status;
      assertEquals(40, country.getDisplaySize(true));
      assertEquals(40, country.getDisplaySize(false));
    }
  }

  @Test
  public void activeEntriesShowCombinedBatchSize()
  {
    CountryItem country = new CountryItem("France");
    country.size = 130;
    country.totalSize = 500;
    country.downloadSize = 40;
    country.updateProgress(new MapManager.Progress(70, 70, 15, 30));
    for (int status : new int[] {CountryItem.STATUS_ENQUEUED, CountryItem.STATUS_PROGRESS, CountryItem.STATUS_APPLYING})
    {
      country.status = status;
      assertEquals(100, country.getDisplaySize(true));
      assertEquals(100, country.getDisplaySize(false));
    }
  }

  @Test
  public void terrainProgressLeavesCompletedMapIntact()
  {
    CountryItem country = new CountryItem("France");
    country.status = CountryItem.STATUS_DONE;
    country.updateProgress(70, 70);

    country.terrainStatus = CountryItem.TERRAIN_PROGRESS;
    country.updateTerrainProgress(10, 30);

    assertEquals(CountryItem.STATUS_DONE, country.status);
    assertEquals(70, country.downloadedBytes);
    assertEquals(70, country.bytesToDownload);
    assertEquals(95.0f, country.progress, 0.01f);
    assertEquals(95.0f / 3, country.terrainProgress, 0.01f);
  }

  @Test
  public void emptyTerrainProgressPreservesInstalledSizeAndStatus()
  {
    CountryItem country = new CountryItem("France");
    country.status = CountryItem.STATUS_DONE;
    country.size = 70;
    country.terrainStatus = CountryItem.TERRAIN_DONE;
    country.terrainSize = 30;
    country.updateTerrainProgress(30, 30);

    country.updateTerrainProgress(0, 0);

    assertEquals(CountryItem.STATUS_DONE, country.status);
    assertEquals(CountryItem.TERRAIN_DONE, country.terrainStatus);
    assertEquals(70, country.size);
    assertEquals(30, country.terrainSize);
  }

  @Test
  public void simultaneousMapProgressLeavesTerrainIntact()
  {
    CountryItem country = new CountryItem("France");
    country.terrainStatus = CountryItem.TERRAIN_PROGRESS;
    country.updateTerrainProgress(15, 30);

    country.status = CountryItem.STATUS_PROGRESS;
    country.updateProgress(7, 70);

    assertEquals(CountryItem.TERRAIN_PROGRESS, country.terrainStatus);
    assertEquals(15, country.terrainDownloadedBytes);
    assertEquals(30, country.terrainBytesToDownload);
    assertEquals(47.5f, country.terrainProgress, 0.01f);
    assertEquals(9.5f, country.progress, 0.01f);
  }

  @Test
  public void unifiedSnapshotCombinesMapAndTerrainProgress()
  {
    CountryItem country = new CountryItem("France");
    country.status = CountryItem.STATUS_PROGRESS;
    country.terrainStatus = CountryItem.TERRAIN_PROGRESS;

    country.updateProgress(new MapManager.Progress(7, 70, 15, 30));

    assertEquals(CountryItem.STATUS_PROGRESS, country.status);
    assertEquals(CountryItem.TERRAIN_PROGRESS, country.terrainStatus);
    assertEquals(22, country.downloadedBytes);
    assertEquals(100, country.bytesToDownload);
    assertEquals(15, country.terrainDownloadedBytes);
    assertEquals(30, country.terrainBytesToDownload);
    assertEquals(20.9f, country.progress, 0.01f);
    assertEquals(47.5f, country.terrainProgress, 0.01f);
  }

  @Test
  public void unifiedSnapshotLeavesCompletedMapAndInstalledSizesIntact()
  {
    CountryItem country = new CountryItem("France");
    country.status = CountryItem.STATUS_DONE;
    country.size = 70;
    country.terrainStatus = CountryItem.TERRAIN_PROGRESS;
    country.terrainSize = 30;

    country.updateProgress(new MapManager.Progress(0, 0, 10, 30));

    assertEquals(CountryItem.STATUS_DONE, country.status);
    assertEquals(70, country.size);
    assertEquals(30, country.terrainSize);
    assertEquals(30, country.bytesToDownload);
    assertEquals(10, country.terrainDownloadedBytes);
    assertEquals(30, country.terrainBytesToDownload);
  }

  @Test
  public void unifiedIdleSnapshotPreservesStatusesAndInstalledSizes()
  {
    CountryItem country = new CountryItem("France");
    country.status = CountryItem.STATUS_DONE;
    country.size = 70;
    country.terrainStatus = CountryItem.TERRAIN_DONE;
    country.terrainSize = 30;

    country.updateProgress(new MapManager.Progress(0, 0, 0, 0));

    assertEquals(CountryItem.STATUS_DONE, country.status);
    assertEquals(CountryItem.TERRAIN_DONE, country.terrainStatus);
    assertEquals(70, country.size);
    assertEquals(30, country.terrainSize);
    assertEquals(0, country.bytesToDownload);
    assertEquals(0, country.terrainBytesToDownload);
  }

  @Test
  public void terrainContinuesTheSameProgressAfterMapCompletion()
  {
    CountryItem country = new CountryItem("France");
    country.status = CountryItem.STATUS_PROGRESS;
    country.updateProgress(new MapManager.Progress(70, 70, 0, 30));
    assertEquals(66.5f, country.progress, 0.01f);
    assertEquals(100, country.bytesToDownload);

    country.updateProgress(new MapManager.Progress(70, 70, 15, 30));
    assertEquals(80.75f, country.progress, 0.01f);
    assertEquals(100, country.bytesToDownload);

    country.updateProgress(new MapManager.Progress(70, 70, 30, 30));
    assertEquals(95.0f, country.progress, 0.01f);
  }
}
