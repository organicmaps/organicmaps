package app.organicmaps.search;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import app.organicmaps.sdk.search.Popularity;
import app.organicmaps.sdk.search.SearchResult;
import org.junit.Test;

public class AddressResultMatcherTest
{
  @Test
  public void matchesCommonAddressAbbreviations()
  {
    assertTrue(AddressResultMatcher.matches("531 W 28th Ave", "531 West 28th Avenue Vancouver BC"));
    assertTrue(AddressResultMatcher.matches("868 West 67 Ave", "868 West 67th Avenue Vancouver"));
    assertTrue(AddressResultMatcher.matches("12A Main St", "12A Main Street Vancouver"));
  }

  @Test
  public void matchesAddressesWithUnitPrefixes()
  {
    final ContactAddress address =
        new ContactAddress("Name", "Home", "Unit 5, 868 West 67 Ave, Vancouver", "Unit 5, 868 West 67 Ave",
                           "Vancouver");
    assertTrue(AddressResultMatcher.matches(address, result("868, West 67th Avenue", "Vancouver")));
    assertFalse(AddressResultMatcher.matches(address, result("868, West 67th Avenue", "Calgary")));
  }

  @Test
  public void rejectsDifferentHouseOrStreet()
  {
    assertFalse(AddressResultMatcher.matches("531 W 28th Ave", "535 West 28th Avenue Vancouver BC"));
    assertFalse(AddressResultMatcher.matches("531 W 28th Ave", "531 West 29th Avenue Vancouver BC"));
  }

  @Test
  public void matchesOnlyTheRequestedNearbyHouseOnTheSameStreet()
  {
    final ContactAddress address = new ContactAddress("Name", "Home", "6498 131a St, Surrey", "6498 131a St", "Surrey");
    assertEquals(6, AddressResultMatcher.houseNumberDifference(address, "6498 131a Street",
                                                               result("6492, 131A Street", "Surrey")));
    assertEquals(Integer.MAX_VALUE,
                 AddressResultMatcher.houseNumberDifference(address, "6498 131a Street",
                                                            result("6492, 133A Street", "Surrey")));
    assertEquals(Integer.MAX_VALUE,
                 AddressResultMatcher.houseNumberDifference(address, "6498 131a Street",
                                                            result("6492, 131A Street", "Vancouver")));
  }

  @Test
  public void requiresAHouseNumber()
  {
    assertFalse(AddressResultMatcher.matches("West 28th Avenue", "West 28th Avenue Vancouver BC"));
  }

  @Test
  public void findsUniqueNonEstimatedExactAddress()
  {
    final SearchResult[] results = {result("868, West 67th Avenue", "Vancouver"),
                                    result("870, West 67th Avenue", "Vancouver")};
    assertEquals(0, AddressResultMatcher.findUniqueExactAddress("868 West 67 Ave Vancouver", results));
  }

  @Test
  public void rejectsEstimatedOrAmbiguousExactAddress()
  {
    final SearchResult estimated = result("868, West 67th Avenue", "Vancouver", true);
    assertEquals(-1,
                 AddressResultMatcher.findUniqueExactAddress("868 West 67 Ave Vancouver", new SearchResult[] {
                     estimated,
                 }));
    assertEquals(-1, AddressResultMatcher.findUniqueExactAddress(
                         "868 West 67 Ave", new SearchResult[] {result("868, West 67th Avenue", "Vancouver"),
                                                               result("868, West 67th Avenue", "Calgary")}));
  }

  private static SearchResult result(String name, String region)
  {
    return result(name, region, false);
  }

  private static SearchResult result(String name, String region, boolean estimated)
  {
    final SearchResult.Description description = new SearchResult.Description("", region, null, "", 0, 0, 0, false);
    return new SearchResult(name, description, 1.0, 1.0, new int[] {}, new int[] {}, Popularity.defaultInstance(),
                            estimated);
  }
}
