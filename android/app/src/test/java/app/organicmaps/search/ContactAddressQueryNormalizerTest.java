package app.organicmaps.search;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import org.junit.Test;

public class ContactAddressQueryNormalizerTest
{
  @Test
  public void normalizesDirectionalNumberedStreets()
  {
    assertEquals("868 West 67th Avenue", ContactAddressQueryNormalizer.normalizeStreet("868 West 67 Ave"));
    assertEquals("531 West 28th Avenue", ContactAddressQueryNormalizer.normalizeStreet("531 W 28th Ave"));
    assertEquals("10 East 11th Street", ContactAddressQueryNormalizer.normalizeStreet("10 E 11 St"));
    assertEquals("10 West 12th Avenue", ContactAddressQueryNormalizer.normalizeStreet("10 W 12 Ave"));
    assertEquals("10 West 23rd Avenue", ContactAddressQueryNormalizer.normalizeStreet("10 W 23 Ave"));
  }

  @Test
  public void preservesNonOrdinalNumberedAndNamedStreets()
  {
    assertEquals("6498 131a Street", ContactAddressQueryNormalizer.normalizeStreet("6498 131a St"));
    assertEquals("100 67 Avenue", ContactAddressQueryNormalizer.normalizeStreet("100 67 Ave"));
    assertEquals("1600 Pennsylvania Avenue", ContactAddressQueryNormalizer.normalizeStreet("1600 Pennsylvania Ave"));
  }

  @Test
  public void removesCommonUnitPrefixes()
  {
    assertEquals("868 West 67th Avenue", ContactAddressQueryNormalizer.normalizeStreet("Unit 5, 868 West 67 Ave"));
    assertEquals("868 West 67th Avenue", ContactAddressQueryNormalizer.normalizeStreet("5-868 West 67 Ave"));
    assertEquals("868 West 67th Avenue", ContactAddressQueryNormalizer.normalizeStreet("#5 868 West 67 Ave"));
  }

  @Test
  public void removesTrailingUnitNumbers()
  {
    assertEquals("2377 West 5th Avenue", ContactAddressQueryNormalizer.normalizeStreet("2377 W 5th Ave #303"));
    assertEquals("2377 West 5th Avenue", ContactAddressQueryNormalizer.normalizeStreet("2377 W 5th Ave Apt 303"));
  }

  @Test
  public void identifiesSimpleAddressQueries()
  {
    assertTrue(ContactAddressQueryNormalizer.looksLikeAddressQuery("6498 131a St, Surrey"));
    assertTrue(ContactAddressQueryNormalizer.looksLikeAddressQuery("2377 W 5th Ave #303, Vancouver"));
    assertFalse(ContactAddressQueryNormalizer.looksLikeAddressQuery("131a St, Surrey"));
    assertFalse(ContactAddressQueryNormalizer.looksLikeAddressQuery("restaurants"));
  }

  @Test
  public void normalizesFullCanadianAddresses()
  {
    assertEquals("16291 111A Avenue Surrey",
                 ContactAddressQueryNormalizer.normalizeAddressQuery(
                     "[16291, 111A Avenue Surrey BC V4N4R7 Canada]"));
    assertEquals("6498 131a Street Surrey",
                 ContactAddressQueryNormalizer.normalizeAddressQuery("6498 131a St, Surrey, BC V3W 7P4, Canada"));
    assertEquals("531 West 28th Avenue Vancouver",
                 ContactAddressQueryNormalizer.normalizeAddressQuery(
                     "531 W 28th Ave, Vancouver, BC V5Z 2H2, Canada"));
  }

  @Test
  public void removesUnitsFromFullAddressQueries()
  {
    assertEquals("2377 West 5th Avenue Vancouver",
                 ContactAddressQueryNormalizer.normalizeAddressQuery(
                     "2377 W 5th Ave #303, Vancouver, BC V6K 1S6, Canada"));
    assertEquals("2377 West 5th Avenue Vancouver",
                 ContactAddressQueryNormalizer.normalizeAddressQuery("303-2377 W 5th Ave, Vancouver"));
    assertEquals("2377 West 5th Avenue Vancouver",
                 ContactAddressQueryNormalizer.normalizeAddressQuery("303/2377 W 5th Ave, Vancouver"));
  }

  @Test
  public void normalizesUsAddressesWithoutConfusingStateAbbreviationsWithStreetSuffixes()
  {
    assertEquals("1600 Pennsylvania Avenue Northwest Washington",
                 ContactAddressQueryNormalizer.normalizeAddressQuery(
                     "1600 Pennsylvania Ave. NW, Washington, DC 20500, USA"));
    assertEquals("50 Main Street Hartford",
                 ContactAddressQueryNormalizer.normalizeAddressQuery("50 Main St, Hartford, CT 06103"));
  }

  @Test
  public void preservesNonAddressQueries()
  {
    assertEquals("restaurants, Surrey", ContactAddressQueryNormalizer.normalizeAddressQuery("restaurants, Surrey"));
  }
}
