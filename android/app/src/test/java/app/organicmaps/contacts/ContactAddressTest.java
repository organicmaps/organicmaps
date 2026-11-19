package app.organicmaps.contacts;

import static org.junit.Assert.*;

import java.util.List;
import org.junit.Test;

public class ContactAddressTest
{
  @Test
  public void preservesQueensCivicNumbersAndBritishMapContext()
  {
    final ContactAddress compound = new ContactAddress("12-34 56th St, Queens, NY, USA", "", "");
    final ContactAddress ordinary = new ContactAddress("34 56th St, Queens, NY, USA", "", "");
    assertEquals("12-34 56th Street", compound.getNormalizedStreet());
    assertNotEquals(compound.getAddressKey(), ordinary.getAddressKey());
    assertNotEquals(compound.getSearchQueries(), ordinary.getSearchQueries());
    assertTrue(ContactAddressNormalizer.matchesMapRegion("London United Kingdom", "UK_England_Greater London"));
    assertFalse(ContactAddressNormalizer.matchesMapRegion("London United Kingdom", "US_New York"));
  }
  @Test
  public void preservesStreetAndGeographicalContext()
  {
    final ContactAddress address =
        new ContactAddress("6498 131a St, Surrey, BC V3W 7P4, Canada", "6498 131a St", "Surrey");
    assertEquals(
        List.of(new ContactAddress.SearchQuery("6498 131a Street Surrey British Columbia Canada", "6498 131a Street")),
        address.getSearchQueries());
  }

  @Test
  public void formattedOnlyAddressPreservesLocality()
  {
    final ContactAddress address = new ContactAddress("868 West 67 Ave, Vancouver", "", "");
    assertEquals(List.of(new ContactAddress.SearchQuery("868 West 67 Ave, Vancouver", "868 West 67 Ave")),
                 address.getSearchQueries());
  }

  @Test
  public void resolvesAbbreviatedStructuredAndFormattedLocalities()
  {
    final ContactAddress structured =
        new ContactAddress("8877 Wright St, Langley Twp, BC, Canada", "8877 Wright St", "Langley Twp", "BC", "Canada");
    final ContactAddress formatted = new ContactAddress("8877 Wright St, Langley TWP., BC, Canada", "", "");
    final ContactAddress expanded = new ContactAddress("8877 Wright St, Langley Township, BC, Canada", "8877 Wright St",
                                                       "Langley Township", "BC", "Canada");
    assertEquals("8877 Wright Street Langley Township British Columbia Canada",
                 structured.getSearchQueries().get(0).query);
    assertEquals(structured.getSearchQueries(), formatted.getSearchQueries());
    assertEquals(structured.getResolutionContext(), formatted.getResolutionContext());
    assertEquals(structured.getAddressKey(), expanded.getAddressKey());
  }

  @Test
  public void handlesFormattedGermanStreetBeforeCityAndPostcode()
  {
    var address = new ContactAddress("Beispielstra\u00dfe 21 Beispielstadt 12345 Germany", "", "");
    assertFalse(address.getSearchQueries().isEmpty());
    assertEquals("Beispielstra\u00dfe 21", address.getNormalizedStreet());
    assertTrue(address.getSearchQueries().get(0).query.contains("beispielstadt"));
  }

  @Test
  public void handlesHyphenatedNumberedStreetAndMultilineMall()
  {
    var hyphenated = new ContactAddress("12345-123 Street\nExample City, BC, Canada", "", "");
    assertFalse(hyphenated.getSearchQueries().isEmpty());
    assertEquals("12345 123 Street", hyphenated.getNormalizedStreet());
    var mall = new ContactAddress("1234 West Mall\nExample City BC Canada", "", "");
    assertFalse(mall.getSearchQueries().isEmpty());
    assertEquals("1234 West Mall", mall.getNormalizedStreet());
  }

  @Test
  public void retainsFormattedCityWhenStructuredLocalityIsBlank()
  {
    var address =
        new ContactAddress("123 Example Road, Example City, BC, Canada", "123 Example Road", "", "BC", "Canada");
    assertTrue(address.getResolutionContext().contains("Example City"));
    assertTrue(address.getResolutionContext().contains("British Columbia"));
  }

  @Test
  public void canadianCountryCodeIsNotMistakenForCalifornia()
  {
    var address = new ContactAddress("123 Example Road, CA", "123 Example Road", "", "", "CA");
    assertEquals("Canada", address.getResolutionContext());
    assertEquals("California", ContactAddressNormalizer.normalizeContext("CA", "USA"));
  }

  @Test
  public void distinguishesStatesInCacheAndQueries()
  {
    final ContactAddress illinois = new ContactAddress("123 Main St, Springfield, IL 62701, USA", "", "");
    final ContactAddress massachusetts = new ContactAddress("123 Main St, Springfield, MA 01103, USA", "", "");
    assertNotEquals(illinois.getAddressKey(), massachusetts.getAddressKey());
    assertEquals("123 Main Street Springfield Illinois USA", illinois.getSearchQueries().get(0).query);
    assertEquals("123 Main Street Springfield Massachusetts USA", massachusetts.getSearchQueries().get(0).query);
  }

  @Test
  public void structuredRegionAndCountryAreNotDropped()
  {
    final ContactAddress address = new ContactAddress("123 Main St", "123 Main St", "Springfield", "IL", "USA");
    assertEquals("123 Main Street Springfield Illinois USA", address.getSearchQueries().get(0).query);
    final ContactAddress canada = new ContactAddress("123 Main St", "123 Main St", "Vancouver", "BC", "CA");
    assertEquals("123 Main Street Vancouver British Columbia Canada", canada.getSearchQueries().get(0).query);
  }

  @Test
  public void handlesFormattedFrenchAddressWithoutEnglishSuffix()
  {
    final ContactAddress address = new ContactAddress("12 Rue de Rivoli, Paris, France", "", "");
    assertEquals(List.of(new ContactAddress.SearchQuery("12 Rue de Rivoli, Paris, France", "12 Rue de Rivoli")),
                 address.getSearchQueries());
  }

  @Test
  public void preservesInternationalStreetOrderAndHouseIdentifiers()
  {
    for (String street : List.of("Hauptstraße 12a", "Křižíkova 12/1", "Via Roma 12-14", "улица Ленина 12"))
    {
      final ContactAddress address = new ContactAddress(street + ", City, Germany", street, "City", "", "DE");
      assertEquals(street, address.getNormalizedStreet());
      assertEquals(List.of(new ContactAddress.SearchQuery(street + ", City Germany", street)),
                   address.getSearchQueries());
    }
  }

  @Test
  public void formattedInternationalAddressKeepsNativeNamesAndPostcodeContext()
  {
    final ContactAddress address = new ContactAddress("Kantstraße 12, 10623 Berlin, Germany", "", "");
    assertEquals("Kantstraße 12", address.getNormalizedStreet());
    assertEquals(List.of(new ContactAddress.SearchQuery("Kantstraße 12, 10623 Berlin, Germany", "Kantstraße 12")),
                 address.getSearchQueries());
    final ContactAddress spanish = new ContactAddress("12 Calle N, Madrid, Spain", "12 Calle N", "Madrid", "CA", "ES");
    assertEquals("12 Calle N", spanish.getNormalizedStreet());
    assertEquals("Madrid CA Spain", spanish.getResolutionContext());
  }

  @Test
  public void australianUnitsAreNotCzechHouseIdentifiers()
  {
    final ContactAddress australian =
        new ContactAddress("2/14 Smith Street, Sydney, Australia", "2/14 Smith Street", "Sydney", "", "AU");
    assertEquals("14 Smith Street", australian.getNormalizedStreet());
    final ContactAddress czech =
        new ContactAddress("Křižíkova 12/1, Praha, Czechia", "Křižíkova 12/1", "Praha", "", "CZ");
    assertEquals("Křižíkova 12/1", czech.getNormalizedStreet());
  }

  @Test
  public void handlesEmptyAndNonAddressContactNotes()
  {
    for (String note : List.of("", ",", ",,", "\n", "Same as Mom", "France"))
      assertEquals(List.of(), new ContactAddress(note, "", "").getSearchQueries());
  }

  @Test
  public void unknownCountryPreservesAmbiguousNamesAndHouseIdentifiers()
  {
    for (String street : List.of("12 Calle N", "Křižíkova 12/1", "Via Roma 12-14"))
    {
      final ContactAddress address = new ContactAddress(street + ", City, CA, España", street, "");
      assertEquals(street, address.getNormalizedStreet());
      assertEquals("City, CA, España", address.getResolutionContext());
    }
    assertEquals("", ContactAddressNormalizer.formattingCountry("", ",,"));
    assertEquals("Spain", ContactAddressNormalizer.formattingCountry("", "12 Calle N, V4N 5R2, Spain"));
  }

  @Test
  public void removesExplicitUnitAndRetriesAmbiguousUnit()
  {
    final ContactAddress explicit =
        new ContactAddress("#15 3495 147A Street, Surrey, BC, Canada", "15 3495 147A Street", "Surrey");
    assertEquals("3495 147A Street Surrey British Columbia Canada", explicit.getSearchQueries().get(0).query);
    final ContactAddress ambiguous =
        new ContactAddress("10 578 Corydon Ave, Winnipeg, MB, Canada", "10 578 Corydon Ave", "Winnipeg");
    assertEquals(
        List.of(
            new ContactAddress.SearchQuery("10 578 Corydon Avenue Winnipeg Manitoba Canada", "10 578 Corydon Avenue"),
            new ContactAddress.SearchQuery("578 Corydon Avenue Winnipeg Manitoba Canada", "578 Corydon Avenue")),
        ambiguous.getSearchQueries());
  }

  @Test
  public void excludesApartmentDetailsFromResolutionContext()
  {
    for (String formatted : List.of("200 Klahanie drive apt. 202", "apt. 202 200 Klahanie drive",
                                    "200 Klahanie drive #202", "Apartment 202 200 Klahanie drive"))
    {
      final ContactAddress address = new ContactAddress(formatted, "apt. 202 200 Klahanie drive", "", "", "Canada");
      assertEquals("Canada", address.getResolutionContext());
      assertEquals(List.of(new ContactAddress.SearchQuery("200 Klahanie Drive Canada", "200 Klahanie Drive")),
                   address.getSearchQueries());
    }
    final ContactAddress address = new ContactAddress("apt. 202 200 Klahanie drive, Port Moody, BC, Canada",
                                                      "apt. 202 200 Klahanie drive", "Port Moody");
    assertEquals("200 Klahanie Drive Port Moody British Columbia Canada", address.getSearchQueries().get(0).query);
  }

  @Test
  public void handlesHouseNumberCommaAndOversizedNumbers()
  {
    final ContactAddress comma = new ContactAddress("16291, 111A Avenue Surrey BC V4N4R7 Canada", "", "");
    assertEquals("16291 111A Avenue surrey british columbia canada", comma.getSearchQueries().get(0).query);
    final ContactAddress large = new ContactAddress("999999999999999999999999 Main St, Surrey", "", "");
    assertFalse(large.getSearchQueries().isEmpty());
  }

  @Test
  public void skipsContactsInOtherMapRegions()
  {
    assertTrue(ContactAddressNormalizer.matchesMapRegion("123 main street|surrey british columbia canada",
                                                         "Canada_British Columbia"));
    assertFalse(ContactAddressNormalizer.matchesMapRegion("123 main street|springfield illinois usa",
                                                          "Canada_British Columbia"));
    assertFalse(ContactAddressNormalizer.matchesMapRegion("123 main street|winnipeg manitoba canada",
                                                          "Canada_British Columbia"));
  }
}
