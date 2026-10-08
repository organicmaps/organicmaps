package app.organicmaps.sdk.downloader;

import static org.junit.Assert.assertEquals;

import org.junit.Test;

public class CountryItemTest
{
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
  public void unifiedSnapshotKeepsBothFlowsIndependent()
  {
    CountryItem country = new CountryItem("France");
    country.status = CountryItem.STATUS_PROGRESS;
    country.terrainStatus = CountryItem.TERRAIN_PROGRESS;

    country.updateProgress(new MapManager.Progress(7, 70, 15, 30));

    assertEquals(CountryItem.STATUS_PROGRESS, country.status);
    assertEquals(CountryItem.TERRAIN_PROGRESS, country.terrainStatus);
    assertEquals(7, country.downloadedBytes);
    assertEquals(70, country.bytesToDownload);
    assertEquals(15, country.terrainDownloadedBytes);
    assertEquals(30, country.terrainBytesToDownload);
    assertEquals(9.5f, country.progress, 0.01f);
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
    assertEquals(0, country.bytesToDownload);
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
}
