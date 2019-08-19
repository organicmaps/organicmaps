package com.mapswithme.maps.bookmarks.data;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import java.util.Arrays;
import java.util.List;

public class SortedBlock
{
  @NonNull
  private final String mName;
  @NonNull
  private List<Long> mBookmarkIds;
  @NonNull
  private List<Long> mTrackIds;

  public SortedBlock(@NonNull String name, @NonNull Long[] bookmarkIds,
                     @NonNull Long[] trackIds)
  {
    mName = name;
    mBookmarkIds = Arrays.asList(bookmarkIds);
    mTrackIds = Arrays.asList(trackIds);
  }

  public boolean isBookmarksBlock() { return !mBookmarkIds.isEmpty(); }
  public boolean isTracksBlock() { return !mTrackIds.isEmpty(); }
  @NonNull
  public String getName() { return mName; }
  @NonNull
  public List<Long> getBookmarkIds() { return mBookmarkIds; }
  @NonNull
  public List<Long> getTrackIds() { return mTrackIds; }
}
