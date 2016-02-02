package com.mapswithme.maps.downloader;

/**
 * Info about data to be updated. Created by native code.
 */
public final class UpdateInfo
{
  public final int filesCount;
  public final int totalSize;

  public UpdateInfo(int filesCount, int totalSize)
  {
    this.filesCount = filesCount;
    this.totalSize = totalSize;
  }
}