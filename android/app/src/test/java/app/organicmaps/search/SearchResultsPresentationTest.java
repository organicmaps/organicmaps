package app.organicmaps.search;

import static org.junit.Assert.*;

import org.junit.Test;

public class SearchResultsPresentationTest
{
  @Test
  public void collapsesAndExpandsWithoutLosingTheLimitNotice()
  {
    assertEquals(0, SearchResultsPresentation.bookmarkRows(0, false));
    assertEquals(10, SearchResultsPresentation.bookmarkRows(10, false));
    assertEquals(11, SearchResultsPresentation.bookmarkRows(11, false));
    assertEquals(12, SearchResultsPresentation.bookmarkRows(11, true));
    assertEquals(12, SearchResultsPresentation.bookmarkRows(1000, false));
    assertEquals(1002, SearchResultsPresentation.bookmarkRows(1000, true));
    assertEquals(SearchResultsPresentation.BookmarkRow.RESULT, SearchResultsPresentation.bookmarkRow(9, 1000, false));
    assertEquals(SearchResultsPresentation.BookmarkRow.TOGGLE, SearchResultsPresentation.bookmarkRow(10, 1000, false));
    assertEquals(SearchResultsPresentation.BookmarkRow.LIMIT, SearchResultsPresentation.bookmarkRow(11, 1000, false));
    assertEquals(SearchResultsPresentation.BookmarkRow.LIMIT, SearchResultsPresentation.bookmarkRow(1001, 1000, true));
  }

  @Test
  public void showsMapPlaceholdersBelowBookmarksOnlyWhileMapSearchIsPending()
  {
    assertTrue(SearchResultsPresentation.showMapLoading(5, 0, true));
    assertFalse(SearchResultsPresentation.showMapLoading(0, 0, true));
    assertFalse(SearchResultsPresentation.showMapLoading(5, 1, true));
    assertFalse(SearchResultsPresentation.showMapLoading(5, 0, false));
    assertTrue(SearchResultsPresentation.showFullLoading(0, 0, true, false));
    assertTrue(SearchResultsPresentation.showFullLoading(0, 0, false, true));
    assertFalse(SearchResultsPresentation.showFullLoading(5, 0, true, false));
    assertFalse(SearchResultsPresentation.showFullLoading(0, 1, true, true));
    assertFalse(SearchResultsPresentation.showFullLoading(0, 0, false, false));
  }
}
