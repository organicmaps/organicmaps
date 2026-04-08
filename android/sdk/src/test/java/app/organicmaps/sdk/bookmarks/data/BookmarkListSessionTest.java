package app.organicmaps.sdk.bookmarks.data;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertThrows;
import static org.junit.Assert.assertTrue;

import androidx.annotation.NonNull;
import java.util.ArrayList;
import java.util.List;
import org.junit.Test;

public class BookmarkListSessionTest
{
  @Test
  public void close_is_idempotent()
  {
    FakeNativeBridge bridge = new FakeNativeBridge();
    BookmarkListSession session = new BookmarkListSession(7, bridge);

    session.close();
    session.close();

    assertEquals(1, bridge.destroyCalls);
    assertTrue(session.isClosed());
  }

  @Test
  public void latest_snapshot_is_dispatched_immediately_when_listener_is_set()
  {
    FakeNativeBridge bridge = new FakeNativeBridge();
    BookmarkListSession session = new BookmarkListSession(1, bridge);
    BookmarkListRow[] rows = {BookmarkListRow.section(-2, BookmarkListRow.SectionKind.BOOKMARKS, null),
                              BookmarkListRow.description(-3, "Category", "Description")};

    session.onSnapshotChanged(true, rows);

    List<BookmarkListSnapshot> received = new ArrayList<>();
    session.setListener(received::add);

    assertEquals(1, received.size());
    assertTrue(received.get(0).isLoading());
    assertArrayEquals(rows, received.get(0).getRows());
  }

  @Test
  public void loading_change_preserves_existing_rows()
  {
    FakeNativeBridge bridge = new FakeNativeBridge();
    BookmarkListSession session = new BookmarkListSession(2, bridge);
    BookmarkListRow[] rows = {BookmarkListRow.section(-4, BookmarkListRow.SectionKind.TRACKS, null)};
    session.onSnapshotChanged(false, rows);

    session.onSnapshotChanged(true, null);

    BookmarkListSnapshot snapshot = session.getLatestSnapshot();
    assertTrue(snapshot.isLoading());
    assertArrayEquals(rows, snapshot.getRows());
  }

  @Test
  public void updates_after_close_are_ignored()
  {
    FakeNativeBridge bridge = new FakeNativeBridge();
    BookmarkListSession session = new BookmarkListSession(3, bridge);
    BookmarkListRow[] rows = {BookmarkListRow.section(-5, BookmarkListRow.SectionKind.DESCRIPTION, null)};
    session.onSnapshotChanged(false, rows);
    session.close();

    session.onSnapshotChanged(true, new BookmarkListRow[] {BookmarkListRow.description(-6, "A", "B")});

    BookmarkListSnapshot snapshot = session.getLatestSnapshot();
    assertFalse(snapshot.isLoading());
    assertArrayEquals(rows, snapshot.getRows());
  }

  @Test
  public void snapshots_are_defensively_copied()
  {
    FakeNativeBridge bridge = new FakeNativeBridge();
    BookmarkListSession session = new BookmarkListSession(4, bridge);
    BookmarkListRow original = BookmarkListRow.section(-7, BookmarkListRow.SectionKind.BOOKMARKS, null);
    BookmarkListRow[] rows = {original};

    session.onSnapshotChanged(false, rows);
    rows[0] = BookmarkListRow.description(-8, "Category", "Description");

    BookmarkListSnapshot snapshot = session.getLatestSnapshot();
    assertArrayEquals(new BookmarkListRow[] {original}, snapshot.getRows());

    BookmarkListRow[] exposedRows = snapshot.getRows();
    exposedRows[0] = BookmarkListRow.section(-9, BookmarkListRow.SectionKind.TRACKS, null);
    assertArrayEquals(new BookmarkListRow[] {original}, snapshot.getRows());
  }

  @Test
  public void show_default_search_and_sort_delegate_to_native_bridge()
  {
    FakeNativeBridge bridge = new FakeNativeBridge();
    bridge.searchResult = true;
    BookmarkListSession session = new BookmarkListSession(9, bridge);

    session.showDefault();
    boolean searchStarted = session.search("cafe");
    session.sort(BookmarkCategory.SortingType.BY_NAME, false, 0.0, 0.0);

    assertEquals(1, bridge.showDefaultCalls);
    assertEquals(1, bridge.searchCalls);
    assertEquals("cafe", bridge.lastQuery);
    assertTrue(searchStarted);
    assertEquals(1, bridge.sortCalls);
    assertEquals(BookmarkCategory.SortingType.BY_NAME, bridge.lastSortingType);
  }

  @Test
  public void public_operations_fail_after_close()
  {
    FakeNativeBridge bridge = new FakeNativeBridge();
    BookmarkListSession session = new BookmarkListSession(10, bridge);
    session.close();

    assertThrows(IllegalStateException.class, session::showDefault);
    assertThrows(IllegalStateException.class, () -> session.search("park"));
    assertThrows(IllegalStateException.class,
                 () -> session.sort(BookmarkCategory.SortingType.BY_TYPE, false, 0.0, 0.0));
  }

  @Test
  public void constructor_fails_when_native_session_cannot_be_created()
  {
    FakeNativeBridge bridge = new FakeNativeBridge();
    bridge.nextPtr = 0;

    IllegalStateException exception =
        assertThrows(IllegalStateException.class, () -> new BookmarkListSession(11, bridge));

    assertEquals("Failed to create native bookmark list session.", exception.getMessage());
  }

  private static class FakeNativeBridge implements BookmarkListSession.NativeBridge
  {
    long nextPtr = 42;
    int destroyCalls;
    int showDefaultCalls;
    int searchCalls;
    int sortCalls;
    boolean searchResult;
    String lastQuery;
    int lastSortingType = -1;

    @Override
    public long create(@NonNull BookmarkListSession session, long categoryId)
    {
      return nextPtr++;
    }

    @Override
    public void destroy(long nativePtr)
    {
      destroyCalls++;
    }

    @Override
    public void showDefault(long nativePtr)
    {
      showDefaultCalls++;
    }

    @Override
    public boolean search(long nativePtr, @NonNull String query)
    {
      searchCalls++;
      lastQuery = query;
      return searchResult;
    }

    @Override
    public void sort(long nativePtr, int sortingType, boolean hasMyPosition, double lat, double lon)
    {
      sortCalls++;
      lastSortingType = sortingType;
    }
  }
}
