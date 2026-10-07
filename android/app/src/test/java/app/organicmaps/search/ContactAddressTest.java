package app.organicmaps.search;

import static org.junit.Assert.*;

import java.util.List;
import org.junit.Test;

public class ContactAddressTest
{
  @Test
  public void preservesStreetAndGeographicalContext()
  {
    final ContactAddress address =
        new ContactAddress("Name", "Home", "6498 131a St, Surrey, BC V3W 7P4, Canada", "6498 131a St", "Surrey");
    assertEquals(
        List.of(new ContactAddress.SearchQuery("6498 131a Street Surrey British Columbia Canada", "6498 131a Street")),
        address.getSearchQueries());
  }

  @Test
  public void formattedOnlyAddressPreservesLocality()
  {
    final ContactAddress address = new ContactAddress("Name", "Home", "868 West 67 Ave, Vancouver", "", "");
    assertEquals(List.of(new ContactAddress.SearchQuery("868 West 67th Avenue Vancouver", "868 West 67th Avenue")),
                 address.getSearchQueries());
  }

  @Test
  public void resolvesAbbreviatedStructuredAndFormattedLocalities()
  {
    final ContactAddress structured = new ContactAddress("Name", "Home", "8877 Wright St, Langley Twp, BC, Canada",
                                                         "8877 Wright St", "Langley Twp", "BC", "Canada");
    final ContactAddress formatted =
        new ContactAddress("Name", "Home", "8877 Wright St, Langley TWP., BC, Canada", "", "");
    final ContactAddress expanded = new ContactAddress("Name", "Home", "8877 Wright St, Langley Township, BC, Canada",
                                                       "8877 Wright St", "Langley Township", "BC", "Canada");
    assertEquals("8877 Wright Street Langley Township British Columbia Canada",
                 structured.getSearchQueries().get(0).query);
    assertEquals(structured.getSearchQueries(), formatted.getSearchQueries());
    assertEquals(structured.contextTokens, formatted.contextTokens);
    assertEquals(structured.getAddressKey(), expanded.getAddressKey());
  }

  @Test
  public void distinguishesStatesInCacheAndQueries()
  {
    final ContactAddress illinois = new ContactAddress("A", "", "123 Main St, Springfield, IL 62701", "", "");
    final ContactAddress massachusetts = new ContactAddress("B", "", "123 Main St, Springfield, MA 01103", "", "");
    assertNotEquals(illinois.getAddressKey(), massachusetts.getAddressKey());
    assertEquals("123 Main Street Springfield Illinois", illinois.getSearchQueries().get(0).query);
    assertEquals("123 Main Street Springfield Massachusetts", massachusetts.getSearchQueries().get(0).query);
  }

  @Test
  public void structuredRegionAndCountryAreNotDropped()
  {
    final ContactAddress address =
        new ContactAddress("Name", "", "123 Main St", "123 Main St", "Springfield", "IL", "USA");
    assertEquals("123 Main Street Springfield Illinois USA", address.getSearchQueries().get(0).query);
    final ContactAddress canada = new ContactAddress("Name", "", "123 Main St", "123 Main St", "Vancouver", "BC", "CA");
    assertEquals("123 Main Street Vancouver British Columbia Canada", canada.getSearchQueries().get(0).query);
  }

  @Test
  public void handlesFormattedFrenchAddressWithoutEnglishSuffix()
  {
    final ContactAddress address = new ContactAddress("Name", "", "12 Rue de Rivoli, Paris, France", "", "");
    assertEquals(List.of(new ContactAddress.SearchQuery("12 Rue de Rivoli, Paris, France", "12 Rue de Rivoli")),
                 address.getSearchQueries());
  }

  @Test
  public void preservesInternationalStreetOrderAndHouseIdentifiers()
  {
    for (String street : List.of("Hauptstraße 12a", "Křižíkova 12/1", "Via Roma 12-14", "улица Ленина 12"))
    {
      final ContactAddress address =
          new ContactAddress("Name", "Home", street + ", City, Germany", street, "City", "", "DE");
      assertEquals(street, address.getNormalizedStreet());
      assertEquals(List.of(new ContactAddress.SearchQuery(street + ", City Germany", street)),
                   address.getSearchQueries());
    }
  }

  @Test
  public void formattedInternationalAddressKeepsNativeNamesAndPostcodeContext()
  {
    final ContactAddress address = new ContactAddress("Name", "Home", "Kantstraße 12, 10623 Berlin, Germany", "", "");
    assertEquals("Kantstraße 12", address.getNormalizedStreet());
    assertEquals(List.of(new ContactAddress.SearchQuery("Kantstraße 12, 10623 Berlin, Germany", "Kantstraße 12")),
                 address.getSearchQueries());
    final ContactAddress spanish =
        new ContactAddress("Name", "Home", "12 Calle N, Madrid, Spain", "12 Calle N", "Madrid", "CA", "ES");
    assertEquals("12 Calle N", spanish.getNormalizedStreet());
    assertEquals("Madrid CA Spain", spanish.getResolutionContext());
  }

  @Test
  public void australianUnitsAreNotCzechHouseIdentifiers()
  {
    final ContactAddress australian = new ContactAddress("Name", "Home", "2/14 Smith Street, Sydney, Australia",
                                                         "2/14 Smith Street", "Sydney", "", "AU");
    assertEquals("14 Smith Street", australian.getNormalizedStreet());
    final ContactAddress czech =
        new ContactAddress("Name", "Home", "Křižíkova 12/1, Praha, Czechia", "Křižíkova 12/1", "Praha", "", "CZ");
    assertEquals("Křižíkova 12/1", czech.getNormalizedStreet());
  }

  @Test
  public void handlesEmptyAndNonAddressContactNotes()
  {
    for (String note : List.of("", "Same as Mom", "France"))
      assertEquals(List.of(), new ContactAddress("Name", "", note, "", "").getSearchQueries());
  }

  @Test
  public void removesExplicitUnitAndRetriesAmbiguousUnit()
  {
    final ContactAddress explicit =
        new ContactAddress("Name", "Home", "#15 3495 147A Street, Surrey, BC", "15 3495 147A Street", "Surrey");
    assertEquals("3495 147A Street Surrey British Columbia", explicit.getSearchQueries().get(0).query);
    final ContactAddress ambiguous =
        new ContactAddress("Name", "Home", "10 578 Corydon Ave, Winnipeg, MB", "10 578 Corydon Ave", "Winnipeg");
    assertEquals(
        List.of(new ContactAddress.SearchQuery("10 578 Corydon Avenue Winnipeg Manitoba", "10 578 Corydon Avenue"),
                new ContactAddress.SearchQuery("578 Corydon Avenue Winnipeg Manitoba", "578 Corydon Avenue")),
        ambiguous.getSearchQueries());
  }

  @Test
  public void excludesApartmentDetailsFromResolutionContext()
  {
    for (String formatted : List.of("200 Klahanie drive apt. 202", "apt. 202 200 Klahanie drive",
                                    "200 Klahanie drive #202", "Apartment 202 200 Klahanie drive"))
    {
      final ContactAddress address = new ContactAddress("Name", "Home", formatted, "apt. 202 200 Klahanie drive", "");
      assertEquals("", address.getResolutionContext());
      assertEquals(List.of(new ContactAddress.SearchQuery("200 Klahanie Drive", "200 Klahanie Drive")),
                   address.getSearchQueries());
    }
    final ContactAddress address = new ContactAddress("Name", "Home", "apt. 202 200 Klahanie drive, Port Moody, BC",
                                                      "apt. 202 200 Klahanie drive", "Port Moody");
    assertEquals("200 Klahanie Drive Port Moody British Columbia", address.getSearchQueries().get(0).query);
  }

  @Test
  public void handlesHouseNumberCommaAndOversizedNumbers()
  {
    final ContactAddress comma = new ContactAddress("Name", "", "16291, 111A Avenue Surrey BC V4N4R7 Canada", "", "");
    assertEquals("16291 111A Avenue surrey british columbia canada", comma.getSearchQueries().get(0).query);
    final ContactAddress large = new ContactAddress("Name", "", "999999999999999999999999 Main St, Surrey", "", "");
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
