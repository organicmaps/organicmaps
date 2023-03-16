package app.organicmaps.bookmarks.data;

import androidx.annotation.NonNull;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class SortedBlock
{
  @NonNull
  private final String mName;
  @NonNull
  private final List<Long> mBookmarkIds;
  @NonNull
  private final List<Long> mTrackIds;

  public SortedBlock(@NonNull String name, @NonNull Long[] bookmarkIds,
                     @NonNull Long[] trackIds)
  {
    mName = name;
    mBookmarkIds = new ArrayList<>(Arrays.asList(bookmarkIds));
    mTrackIds = new ArrayList<>(Arrays.asList(trackIds));
  }

  public boolean isBookmarksBlock() { return !mBookmarkIds.isEmpty(); }
  public boolean isTracksBlock() { return !mTrackIds.isEmpty(); }
  @NonNull
  public String getName() { return mName; }

  @SuppressWarnings("AssignmentOrReturnOfFieldWithMutableType")
  @NonNull
  public List<Long> getBookmarkIds() { return mBookmarkIds; }

  @SuppressWarnings("AssignmentOrReturnOfFieldWithMutableType")
  @NonNull
  public List<Long> getTrackIds() { return mTrackIds; }
}
