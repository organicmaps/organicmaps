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
  public void unifiedSnapshotCombinesMapAndTerrainProgress()
  {
    CountryItem country = new CountryItem("France");
    country.status = CountryItem.STATUS_PROGRESS;

    country.updateProgress(new MapManager.Progress(7, 70, 15, 30));

    assertEquals(CountryItem.STATUS_PROGRESS, country.status);
    assertEquals(22, country.downloadedBytes);
    assertEquals(100, country.bytesToDownload);
    assertEquals(20.9f, country.progress, 0.01f);
  }

  @Test
  public void unifiedSnapshotLeavesCompletedMapAndInstalledSizesIntact()
  {
    CountryItem country = new CountryItem("France");
    country.status = CountryItem.STATUS_DONE;
    country.size = 70;

    country.updateProgress(new MapManager.Progress(0, 0, 10, 30));

    assertEquals(CountryItem.STATUS_DONE, country.status);
    assertEquals(70, country.size);
    assertEquals(30, country.bytesToDownload);
  }

  @Test
  public void unifiedIdleSnapshotPreservesStatusesAndInstalledSizes()
  {
    CountryItem country = new CountryItem("France");
    country.status = CountryItem.STATUS_DONE;
    country.size = 70;

    country.updateProgress(new MapManager.Progress(0, 0, 0, 0));

    assertEquals(CountryItem.STATUS_DONE, country.status);
    assertEquals(70, country.size);
    assertEquals(0, country.bytesToDownload);
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
