package app.organicmaps.sdk.bookmarks.data;

import androidx.annotation.NonNull;

public final class BookmarkImportResult
{
  @NonNull
  private final long[] mCategoryIds;
  @NonNull
  private final String[] mFailedFileNames;

  BookmarkImportResult(@NonNull long[] categoryIds, @NonNull String[] failedFileNames)
  {
    mCategoryIds = categoryIds.clone();
    mFailedFileNames = failedFileNames.clone();
  }

  @NonNull
  public long[] getCategoryIds()
  {
    return mCategoryIds.clone();
  }

  @NonNull
  public String[] getFailedFileNames()
  {
    return mFailedFileNames.clone();
  }
}
