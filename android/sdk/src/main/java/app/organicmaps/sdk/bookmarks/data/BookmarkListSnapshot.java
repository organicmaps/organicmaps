package app.organicmaps.sdk.bookmarks.data;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.VisibleForTesting;
import java.util.Arrays;

@Keep
@SuppressWarnings("unused")
public final class BookmarkListSnapshot
{
  @NonNull
  public static final BookmarkListSnapshot EMPTY = new BookmarkListSnapshot(false, new BookmarkListRow[0]);

  private final boolean mLoading;
  @NonNull
  private final BookmarkListRow[] mRows;

  @Keep
  BookmarkListSnapshot(boolean loading, @NonNull BookmarkListRow[] rows)
  {
    mLoading = loading;
    mRows = rows.clone();
  }

  public boolean isLoading()
  {
    return mLoading;
  }

  public int size()
  {
    return mRows.length;
  }

  @NonNull
  public BookmarkListRow getRow(int index)
  {
    return mRows[index];
  }

  @NonNull
  public BookmarkListRow[] getRows()
  {
    return mRows.clone();
  }

  @VisibleForTesting
  @NonNull
  public static BookmarkListSnapshot forTest(boolean loading, @NonNull BookmarkListRow... rows)
  {
    return new BookmarkListSnapshot(loading, rows);
  }

  @Override
  public boolean equals(Object o)
  {
    if (this == o)
      return true;
    if (o == null || getClass() != o.getClass())
      return false;
    BookmarkListSnapshot that = (BookmarkListSnapshot) o;
    return mLoading == that.mLoading && Arrays.equals(mRows, that.mRows);
  }

  @Override
  public int hashCode()
  {
    return 31 * Boolean.hashCode(mLoading) + Arrays.hashCode(mRows);
  }
}
