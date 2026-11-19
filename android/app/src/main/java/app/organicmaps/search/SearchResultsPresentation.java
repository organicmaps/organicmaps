package app.organicmaps.search;

/** Pure presentation rules: bookmark matches must not bury the map results. */
final class SearchResultsPresentation
{
  static final int INITIAL_BOOKMARKS = 10;
  // Matches SearchParams::kDefaultNumBookmarksResults; the native API enforces the query limit.
  static final int BOOKMARK_LIMIT = 1000;

  enum BookmarkRow
  {
    RESULT,
    TOGGLE,
    LIMIT
  }

  private SearchResultsPresentation() {}

  static int visibleBookmarks(int count, boolean expanded)
  {
    return expanded ? count : Math.min(INITIAL_BOOKMARKS, count);
  }

  static int bookmarkRows(int count, boolean expanded)
  {
    return visibleBookmarks(count, expanded) + (count > INITIAL_BOOKMARKS ? 1 : 0) + (count >= BOOKMARK_LIMIT ? 1 : 0);
  }

  static BookmarkRow bookmarkRow(int position, int count, boolean expanded)
  {
    if (position < 0 || position >= bookmarkRows(count, expanded))
      throw new IndexOutOfBoundsException(Integer.toString(position));
    int visible = visibleBookmarks(count, expanded);
    if (position < visible)
      return BookmarkRow.RESULT;
    return position == visible ? BookmarkRow.TOGGLE : BookmarkRow.LIMIT;
  }

  static boolean showMapLoading(int bookmarks, int mapResults, boolean mapRunning)
  {
    return mapRunning && bookmarks > 0 && mapResults == 0;
  }

  static boolean showFullLoading(int bookmarks, int mapResults, boolean mapRunning, boolean bookmarksRunning)
  {
    return bookmarks == 0 && mapResults == 0 && (mapRunning || bookmarksRunning);
  }
}
