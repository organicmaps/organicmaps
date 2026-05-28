package app.organicmaps.search;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNull;

import org.junit.Test;

public class SearchRequestTest
{
  @Test
  public void storesAllFields()
  {
    final SearchRequest request = new SearchRequest("cafe", "en", SearchRequest.Mode.MAP_ONLY);
    assertEquals("cafe", request.query);
    assertEquals("en", request.locale);
    assertEquals(SearchRequest.Mode.MAP_ONLY, request.mode);
  }

  @Test
  public void allowsNullQueryAndLocaleForEmptySearch()
  {
    final SearchRequest request = new SearchRequest(null, null, SearchRequest.Mode.SHEET);
    assertNull(request.query);
    assertNull(request.locale);
    assertEquals(SearchRequest.Mode.SHEET, request.mode);
  }
}
