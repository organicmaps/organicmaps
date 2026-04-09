package app.organicmaps.sdk.bookmarks.data;

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

    session.onSnapshotChanged(true, new int[] {BookmarkListRow.Type.SECTION, BookmarkListRow.Type.DESCRIPTION},
                              new long[] {-2, -3},
                              new int[] {BookmarkListRow.SectionKind.BOOKMARKS, BookmarkListRow.SectionKind.NONE});

    List<BookmarkListSnapshot> received = new ArrayList<>();
    session.setListener(received::add);

    assertEquals(1, received.size());
    assertTrue(received.get(0).isLoading());
    assertEquals(2, received.get(0).size());
    assertEquals(BookmarkListRow.Type.SECTION, received.get(0).getType(0));
    assertEquals(BookmarkListRow.Type.DESCRIPTION, received.get(0).getType(1));
  }

  @Test
  public void loading_change_with_null_arrays_preserves_empty_snapshot()
  {
    FakeNativeBridge bridge = new FakeNativeBridge();
    BookmarkListSession session = new BookmarkListSession(2, bridge);
    session.onSnapshotChanged(false, new int[] {BookmarkListRow.Type.SECTION}, new long[] {-4},
                              new int[] {BookmarkListRow.SectionKind.TRACKS});

    // Null arrays signal loading state without new data.
    session.onSnapshotChanged(true, null, null, null);

    BookmarkListSnapshot snapshot = session.getLatestSnapshot();
    assertTrue(snapshot.isLoading());
    assertEquals(0, snapshot.size());
  }

  @Test
  public void updates_after_close_are_ignored()
  {
    FakeNativeBridge bridge = new FakeNativeBridge();
    BookmarkListSession session = new BookmarkListSession(3, bridge);
    session.onSnapshotChanged(false, new int[] {BookmarkListRow.Type.SECTION}, new long[] {-5},
                              new int[] {BookmarkListRow.SectionKind.DESCRIPTION});
    session.close();

    session.onSnapshotChanged(true, new int[] {BookmarkListRow.Type.DESCRIPTION}, new long[] {-6},
                              new int[] {BookmarkListRow.SectionKind.NONE});

    BookmarkListSnapshot snapshot = session.getLatestSnapshot();
    assertFalse(snapshot.isLoading());
    assertEquals(1, snapshot.size());
    assertEquals(BookmarkListRow.Type.SECTION, snapshot.getType(0));
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
    assertThrows(IllegalStateException.class, () -> session.getRow(0));
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

  @Test
  public void getRow_delegates_to_native_bridge()
  {
    FakeNativeBridge bridge = new FakeNativeBridge();
    BookmarkListSession session = new BookmarkListSession(12, bridge);

    BookmarkListRow row = session.getRow(5);

    assertEquals(1, bridge.getRowCalls);
    assertEquals(5, bridge.lastGetRowIndex);
    assertEquals(BookmarkListRow.Type.SECTION, row.getType());
  }

  private static class FakeNativeBridge implements BookmarkListSession.NativeBridge
  {
    long nextPtr = 42;
    int destroyCalls;
    int showDefaultCalls;
    int searchCalls;
    int sortCalls;
    int getRowCalls;
    int lastGetRowIndex = -1;
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

    @NonNull
    @Override
    public BookmarkListRow getRow(long nativePtr, int index)
    {
      getRowCalls++;
      lastGetRowIndex = index;
      return BookmarkListRow.section(index, BookmarkListRow.SectionKind.BOOKMARKS, null);
    }
  }
}
