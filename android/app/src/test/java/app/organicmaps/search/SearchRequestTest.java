package app.organicmaps.search;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

import org.junit.Test;

public class SearchRequestTest
{
  @Test
  public void storesAllFields()
  {
    final SearchRequest request = new SearchRequest("cafe", "en", true);
    assertEquals("cafe", request.query);
    assertEquals("en", request.locale);
    assertTrue(request.isCategory);
  }

  @Test
  public void allowsNullQueryAndLocaleForEmptySearch()
  {
    final SearchRequest request = new SearchRequest(null, null);
    assertNull(request.query);
    assertNull(request.locale);
    assertFalse(request.isCategory);
  }
}
