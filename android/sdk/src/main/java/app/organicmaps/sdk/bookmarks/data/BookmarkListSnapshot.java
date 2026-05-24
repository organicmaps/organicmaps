package app.organicmaps.sdk.bookmarks.data;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.VisibleForTesting;

/**
 * Lightweight metadata snapshot of a bookmark list session.
 * Contains only structural information (row types, stable IDs, section kinds).
 * Full row content is fetched lazily via {@link BookmarkListSession#getRow(int)}.
 */
@Keep
@SuppressWarnings("unused")
public final class BookmarkListSnapshot
{
  @NonNull
  public static final BookmarkListSnapshot EMPTY = new BookmarkListSnapshot(false, new int[0], new long[0], new int[0]);

  private final boolean mLoading;
  @NonNull
  private final int[] mTypes;
  @NonNull
  private final long[] mStableIds;
  @NonNull
  private final int[] mSectionKinds;

  @Keep
  BookmarkListSnapshot(boolean loading, @NonNull int[] types, @NonNull long[] stableIds, @NonNull int[] sectionKinds)
  {
    mLoading = loading;
    mTypes = types;
    mStableIds = stableIds;
    mSectionKinds = sectionKinds;
  }

  public boolean isLoading()
  {
    return mLoading;
  }

  public int size()
  {
    return mTypes.length;
  }

  @BookmarkListRow.Type
  public int getType(int index)
  {
    return mTypes[index];
  }

  public long getStableId(int index)
  {
    return mStableIds[index];
  }

  @BookmarkListRow.SectionKind
  public int getSectionKind(int index)
  {
    return mSectionKinds[index];
  }

  @NonNull
  BookmarkListSnapshot withLoading(boolean loading)
  {
    if (loading == mLoading)
      return this;
    return new BookmarkListSnapshot(loading, mTypes, mStableIds, mSectionKinds);
  }

  @VisibleForTesting
  @NonNull
  public static BookmarkListSnapshot forTest(boolean loading, @NonNull int[] types, @NonNull long[] stableIds,
                                             @NonNull int[] sectionKinds)
  {
    return new BookmarkListSnapshot(loading, types, stableIds, sectionKinds);
  }
}
