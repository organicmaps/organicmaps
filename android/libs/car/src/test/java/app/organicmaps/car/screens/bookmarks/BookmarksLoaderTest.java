package app.organicmaps.car.screens.bookmarks;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertSame;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

import app.organicmaps.sdk.bookmarks.data.BookmarkInfo;
import app.organicmaps.sdk.bookmarks.data.BookmarkListRow;
import app.organicmaps.sdk.bookmarks.data.BookmarkListSession;
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

    BookmarkListSession session = mock(BookmarkListSession.class);
    when(session.getRow(2)).thenReturn(BookmarkListRow.bookmark(firstBookmark));
    when(session.getRow(4)).thenReturn(BookmarkListRow.bookmark(secondBookmark));

    BookmarkListSnapshot snapshot = BookmarkListSnapshot.forTest(
        false,
        new int[] {BookmarkListRow.Type.SECTION, BookmarkListRow.Type.DESCRIPTION, BookmarkListRow.Type.BOOKMARK,
                   BookmarkListRow.Type.TRACK, BookmarkListRow.Type.BOOKMARK},
        new long[] {-1, -2, 1, -8, 2},
        new int[] {BookmarkListRow.SectionKind.DESCRIPTION, BookmarkListRow.SectionKind.NONE,
                   BookmarkListRow.SectionKind.NONE, BookmarkListRow.SectionKind.NONE,
                   BookmarkListRow.SectionKind.NONE});

    List<BookmarkInfo> bookmarks = BookmarksLoader.extractBookmarks(session, snapshot, 1);

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

    BookmarkListSession session = mock(BookmarkListSession.class);
    when(session.getRow(1)).thenReturn(BookmarkListRow.bookmark(bookmark));

    BookmarkListSnapshot snapshot = BookmarkListSnapshot.forTest(
        false, new int[] {BookmarkListRow.Type.TRACK, BookmarkListRow.Type.BOOKMARK, BookmarkListRow.Type.SECTION},
        new long[] {-10, 3, -3},
        new int[] {BookmarkListRow.SectionKind.NONE, BookmarkListRow.SectionKind.NONE,
                   BookmarkListRow.SectionKind.BOOKMARKS});

    List<BookmarkInfo> bookmarks = BookmarksLoader.extractBookmarks(session, snapshot, 5);

    assertEquals(1, bookmarks.size());
    assertSame(bookmark, bookmarks.get(0));
  }
}
