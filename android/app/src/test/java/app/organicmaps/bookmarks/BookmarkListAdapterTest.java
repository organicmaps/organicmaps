package app.organicmaps.bookmarks;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertThrows;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

import app.organicmaps.sdk.bookmarks.data.BookmarkInfo;
import app.organicmaps.sdk.bookmarks.data.BookmarkListRow;
import app.organicmaps.sdk.bookmarks.data.BookmarkListSnapshot;
import app.organicmaps.sdk.bookmarks.data.Track;
import java.lang.reflect.Constructor;
import org.junit.Test;

public class BookmarkListAdapterTest
{
  @Test
  public void snapshot_rows_define_adapter_content() throws Exception
  {
    BookmarkInfo bookmark = mock(BookmarkInfo.class);
    when(bookmark.getBookmarkId()).thenReturn(11L);
    Track track = mock(Track.class);
    when(track.getTrackId()).thenReturn(22L);

    BookmarkListAdapter adapter = new BookmarkListAdapter();
    adapter.setSnapshot(snapshot(false, BookmarkListRow.section(-1, BookmarkListRow.SectionKind.DESCRIPTION, null),
                                 BookmarkListRow.description(-2, "Category", "Description"),
                                 BookmarkListRow.bookmark(bookmark), BookmarkListRow.track(track)),
                        false);

    assertEquals(4, adapter.getItemCount());
    assertEquals(BookmarkListAdapter.TYPE_SECTION, adapter.getItemViewType(0));
    assertEquals(BookmarkListAdapter.TYPE_DESC, adapter.getItemViewType(1));
    assertEquals(BookmarkListAdapter.TYPE_BOOKMARK, adapter.getItemViewType(2));
    assertEquals(BookmarkListAdapter.TYPE_TRACK, adapter.getItemViewType(3));
    assertEquals(11L, adapter.getItemId(2));
    assertEquals(-23L, adapter.getItemId(3));
    assertSame(bookmark, adapter.getItem(2));
    assertSame(track, adapter.getItem(3));
  }

  @Test
  public void search_mode_and_position_lookup_follow_snapshot_ids() throws Exception
  {
    BookmarkInfo firstBookmark = mock(BookmarkInfo.class);
    when(firstBookmark.getBookmarkId()).thenReturn(1L);
    BookmarkInfo secondBookmark = mock(BookmarkInfo.class);
    when(secondBookmark.getBookmarkId()).thenReturn(2L);
    Track track = mock(Track.class);
    when(track.getTrackId()).thenReturn(33L);

    BookmarkListAdapter adapter = new BookmarkListAdapter();
    adapter.setSnapshot(snapshot(false, BookmarkListRow.bookmark(firstBookmark),
                                 BookmarkListRow.bookmark(secondBookmark), BookmarkListRow.track(track)),
                        true);

    assertTrue(adapter.isSearchResults());
    assertEquals(1, adapter.getPositionById(2L, BookmarkListAdapter.TYPE_BOOKMARK));
    assertEquals(2, adapter.getPositionById(33L, BookmarkListAdapter.TYPE_TRACK));
    assertEquals(-1, adapter.getPositionById(99L, BookmarkListAdapter.TYPE_BOOKMARK));
  }

  @Test
  public void get_item_rejects_non_content_rows() throws Exception
  {
    BookmarkListAdapter adapter = new BookmarkListAdapter();
    adapter.setSnapshot(snapshot(false, BookmarkListRow.section(-1, BookmarkListRow.SectionKind.BOOKMARKS, null)),
                        false);

    assertThrows(UnsupportedOperationException.class, () -> adapter.getItem(0));
  }

  private static BookmarkListSnapshot snapshot(boolean loading, BookmarkListRow... rows) throws Exception
  {
    Constructor<BookmarkListSnapshot> constructor =
        BookmarkListSnapshot.class.getDeclaredConstructor(boolean.class, BookmarkListRow[].class);
    constructor.setAccessible(true);
    return constructor.newInstance(loading, rows);
  }
}
