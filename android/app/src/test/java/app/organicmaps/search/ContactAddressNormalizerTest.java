package app.organicmaps.search;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import java.util.List;
import org.junit.Test;

public class ContactAddressNormalizerTest
{
  @Test
  public void formatsContactProviderFields()
  {
    assertEquals("123 Main Street, Vancouver BC, Canada",
                 ContactAddressNormalizer.format("123 Main Street\nVancouver BC\r\nCanada"));
    assertEquals("123 Main Street, Vancouver, BC, V1V 1V1, Canada",
                 ContactAddressNormalizer.format(null, "123 Main Street", null, "", "Vancouver", "BC",
                                                       "V1V 1V1", "Canada"));
    assertEquals("Vancouver, BC", ContactAddressNormalizer.format(null, "Vancouver", "Vancouver", "BC"));
  }

  @Test
  public void canonicalizesAddressMatchTokens()
  {
    assertEquals(List.of("531", "west", "28", "avenue"),
                 ContactAddressNormalizer.matchTokens("531 W 28th Ave"));
    assertEquals(List.of("1091", "euphrates", "crescent"),
                 ContactAddressNormalizer.matchTokens("1091 Euphrates Cr."));
  }

  @Test
  public void normalizesDirectionalNumberedStreets()
  {
    assertEquals("868 West 67th Avenue", ContactAddressNormalizer.normalizeStreet("868 West 67 Ave"));
    assertEquals("531 West 28th Avenue", ContactAddressNormalizer.normalizeStreet("531 W 28th Ave"));
    assertEquals("10 East 11th Street", ContactAddressNormalizer.normalizeStreet("10 E 11 St"));
    assertEquals("10 West 12th Avenue", ContactAddressNormalizer.normalizeStreet("10 W 12 Ave"));
    assertEquals("10 West 23rd Avenue", ContactAddressNormalizer.normalizeStreet("10 W 23 Ave"));
  }

  @Test
  public void preservesNonOrdinalNumberedAndNamedStreets()
  {
    assertEquals("6498 131a Street", ContactAddressNormalizer.normalizeStreet("6498 131a St"));
    assertEquals("100 67 Avenue", ContactAddressNormalizer.normalizeStreet("100 67 Ave"));
    assertEquals("1600 Pennsylvania Avenue", ContactAddressNormalizer.normalizeStreet("1600 Pennsylvania Ave"));
  }

  @Test
  public void removesCommonUnitPrefixes()
  {
    assertEquals("868 West 67th Avenue", ContactAddressNormalizer.normalizeStreet("Unit 5, 868 West 67 Ave"));
    assertEquals("868 West 67th Avenue", ContactAddressNormalizer.normalizeStreet("5-868 West 67 Ave"));
    assertEquals("868 West 67th Avenue", ContactAddressNormalizer.normalizeStreet("#5 868 West 67 Ave"));
    assertEquals("10238 155a Street", ContactAddressNormalizer.normalizeStreet("Unit #3 10238 155a Street"));
    assertEquals("2453 163 Street", ContactAddressNormalizer.normalizeStreet("#10 2453 163 St"));
    assertEquals("1400 9th Avenue Southeast", ContactAddressNormalizer.normalizeStreet("1-1400 9th Ave SE"));
    assertEquals("32729 Garibaldi Drive",
                 ContactAddressNormalizer.normalizeStreet("103 - 32729 Garibaldi Drive"));
  }

  @Test
  public void removesTrailingUnitNumbers()
  {
    assertEquals("2377 West 5th Avenue", ContactAddressNormalizer.normalizeStreet("2377 W 5th Ave #303"));
    assertEquals("2377 West 5th Avenue", ContactAddressNormalizer.normalizeStreet("2377 W 5th Ave Apt 303"));
  }

  @Test
  public void identifiesSimpleAddressQueries()
  {
    assertTrue(ContactAddressNormalizer.looksLikeAddressQuery("6498 131a St, Surrey"));
    assertTrue(ContactAddressNormalizer.looksLikeAddressQuery("2377 W 5th Ave #303, Vancouver"));
    assertFalse(ContactAddressNormalizer.looksLikeAddressQuery("131a St, Surrey"));
    assertFalse(ContactAddressNormalizer.looksLikeAddressQuery("restaurants"));
  }

  @Test
  public void normalizesFullCanadianAddresses()
  {
    assertEquals("16291 111A Avenue Surrey",
                 ContactAddressNormalizer.normalizeAddressQuery(
                     "[16291, 111A Avenue Surrey BC V4N4R7 Canada]"));
    assertEquals("6498 131a Street Surrey",
                 ContactAddressNormalizer.normalizeAddressQuery("6498 131a St, Surrey, BC V3W 7P4, Canada"));
    assertEquals("531 West 28th Avenue Vancouver",
                 ContactAddressNormalizer.normalizeAddressQuery(
                     "531 W 28th Ave, Vancouver, BC V5Z 2H2, Canada"));
  }

  @Test
  public void removesUnitsFromFullAddressQueries()
  {
    assertEquals("2377 West 5th Avenue Vancouver",
                 ContactAddressNormalizer.normalizeAddressQuery(
                     "2377 W 5th Ave #303, Vancouver, BC V6K 1S6, Canada"));
    assertEquals("2377 West 5th Avenue Vancouver",
                 ContactAddressNormalizer.normalizeAddressQuery("303-2377 W 5th Ave, Vancouver"));
    assertEquals("2377 West 5th Avenue Vancouver",
                 ContactAddressNormalizer.normalizeAddressQuery("303/2377 W 5th Ave, Vancouver"));
    assertEquals("531 West 28th Avenue Vancouver",
                 ContactAddressNormalizer.normalizeAddressQuery(
                     "531 West 28 Ave, Basement Vancouver, B.C. V5Z 2H2, Canada Canada"));
  }

  @Test
  public void normalizesUsAddressesWithoutConfusingStateAbbreviationsWithStreetSuffixes()
  {
    assertEquals("1600 Pennsylvania Avenue Northwest Washington",
                 ContactAddressNormalizer.normalizeAddressQuery(
                     "1600 Pennsylvania Ave. NW, Washington, DC 20500, USA"));
    assertEquals("50 Main Street Hartford",
                 ContactAddressNormalizer.normalizeAddressQuery("50 Main St, Hartford, CT 06103"));
  }

  @Test
  public void preservesNonAddressQueries()
  {
    assertEquals("restaurants, Surrey", ContactAddressNormalizer.normalizeAddressQuery("restaurants, Surrey"));
    assertFalse(ContactAddressNormalizer.looksLikeAddressQuery("same as Mom"));
    assertFalse(ContactAddressNormalizer.looksLikeAddressQuery("person@example.com"));
  }

  @Test
  public void normalizesCommonContactFormatting()
  {
    assertEquals("10879 160 Street", ContactAddressNormalizer.normalizeStreet("10879-160 Street"));
    assertEquals("14033 92 Avenue", ContactAddressNormalizer.normalizeStreet("14033 - 92 Avenue"));
    assertEquals("11371 161 Street", ContactAddressNormalizer.normalizeStreet("11371\n161 Street"));
    assertEquals("10514 74 Street Northwest", ContactAddressNormalizer.normalizeStreet("10514 74st NW"));
    assertEquals("11093 158 Street", ContactAddressNormalizer.normalizeStreet("11093 158St"));
    assertEquals("12479 79A Avenue", ContactAddressNormalizer.normalizeStreet("12479 79 a ave"));
    assertEquals("76 West 37th Avenue", ContactAddressNormalizer.normalizeStreet("76 w37 avenue"));
    assertEquals("16759 85A Avenue", ContactAddressNormalizer.normalizeStreet("16759 85A 85A Ave"));
    assertEquals("16896 81B Avenue",
                 ContactAddressNormalizer.normalizeStreet(
                     "V4N5E5 Please do not mail anything here 16896 81B Ave"));
    assertEquals("8877 Wright Street Langley Township",
                 ContactAddressNormalizer.normalizeAddressQuery(
                     "8877 Wright St, Langley Twp, BC V1M 3T1, Canada"));
  }

  @Test
  public void generatesConservativeBareUnitFallbacks()
  {
    assertEquals("578 Corydon Avenue",
                 ContactAddressNormalizer.possibleBareUnitStreet("10 578 Corydon Avenue"));
    assertEquals("", ContactAddressNormalizer.possibleBareUnitStreet("6498 131A Street"));
    assertEquals("10 67 Avenue", ContactAddressNormalizer.normalizeStreet("10 67 Avenue"));
  }

  @Test
  public void doesNotExtractUnrelatedNumbersFromNonAddresses()
  {
    assertFalse(ContactAddressNormalizer.looksLikeAddressQuery("Lagjia 3 Ruga e Rehoves Pali Prifti"));
    assertFalse(ContactAddressNormalizer.looksLikeAddressQuery("Please call 12345 tomorrow"));
    assertFalse(ContactAddressNormalizer.looksLikeAddressQuery("5 St Michel"));
    assertFalse(ContactAddressNormalizer.looksLikeAddressQuery("12 Av Diagonal"));
  }

  @Test
  public void doesNotCrashOnOversizedHouseOrUnitNumbers()
  {
    assertEquals("999999999999999999999999 10 Main Street",
                 ContactAddressNormalizer.normalizeStreet("999999999999999999999999-10 Main Street"));
    assertEquals("", ContactAddressNormalizer.possibleBareUnitStreet(
                         "999999999999999999999999 10 Main Street"));
  }
}
