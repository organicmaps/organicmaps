package app.organicmaps.car.screens.bookmarks;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertSame;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

import app.organicmaps.sdk.bookmarks.data.BookmarkInfo;
import app.organicmaps.sdk.bookmarks.data.BookmarkListRow;
import app.organicmaps.sdk.bookmarks.data.BookmarkListSnapshot;
import app.organicmaps.sdk.bookmarks.data.Track;
import java.util.List;
import org.junit.Test;

public class BookmarksLoaderTest
{
  @Test
  public void extractBookmarks_skips_non_bookmark_rows_and_limits_results()
  {
    BookmarkInfo firstBookmark = mock(BookmarkInfo.class);
    when(firstBookmark.getBookmarkId()).thenReturn(1L);
    BookmarkInfo secondBookmark = mock(BookmarkInfo.class);
    when(secondBookmark.getBookmarkId()).thenReturn(2L);
    Track track = mock(Track.class);
    when(track.getTrackId()).thenReturn(7L);

    BookmarkListSnapshot snapshot = BookmarkListSnapshot.forTest(
        false, BookmarkListRow.section(-1, BookmarkListRow.SectionKind.DESCRIPTION, null),
        BookmarkListRow.description(-2, "Category", "Description"), BookmarkListRow.bookmark(firstBookmark),
        BookmarkListRow.track(track), BookmarkListRow.bookmark(secondBookmark));

    List<BookmarkInfo> bookmarks = BookmarksLoader.extractBookmarks(snapshot, 1);

    assertEquals(1, bookmarks.size());
    assertSame(firstBookmark, bookmarks.get(0));
  }

  @Test
  public void extractBookmarks_returns_only_bookmark_rows()
  {
    BookmarkInfo bookmark = mock(BookmarkInfo.class);
    when(bookmark.getBookmarkId()).thenReturn(3L);
    Track track = mock(Track.class);
    when(track.getTrackId()).thenReturn(9L);

    BookmarkListSnapshot snapshot =
        BookmarkListSnapshot.forTest(false, BookmarkListRow.track(track), BookmarkListRow.bookmark(bookmark),
                                     BookmarkListRow.section(-3, BookmarkListRow.SectionKind.BOOKMARKS, null));

    List<BookmarkInfo> bookmarks = BookmarksLoader.extractBookmarks(snapshot, 5);

    assertEquals(1, bookmarks.size());
    assertSame(bookmark, bookmarks.get(0));
  }
}
