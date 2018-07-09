package com.mapswithme.maps.bookmarks.data;

import android.support.annotation.Nullable;

public class Result
{
  @Nullable
  private final String mFilePath;
  @Nullable
  private final String mArchiveId;

  public Result(@Nullable String filePath, @Nullable String archiveId)
  {
    mFilePath = filePath;
    mArchiveId = archiveId;
  }

  @Nullable
  public String getFilePath()
  {
    return mFilePath;
  }

  @Nullable
  public String getArchiveId()
  {
    return mArchiveId;
  }
}
