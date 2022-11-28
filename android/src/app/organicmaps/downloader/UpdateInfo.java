package app.organicmaps.downloader;

/**
 * Info about data to be updated. Created by native code.
 */
public final class UpdateInfo
{
  public final int filesCount;
  public final long totalSize;

  public UpdateInfo(int filesCount, long totalSize)
  {
    this.filesCount = filesCount;
    this.totalSize = totalSize;
  }
}
