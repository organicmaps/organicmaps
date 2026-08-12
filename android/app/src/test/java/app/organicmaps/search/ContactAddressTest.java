package app.organicmaps.search;

import static org.junit.Assert.assertEquals;

import java.util.List;
import org.junit.Test;

public class ContactAddressTest
{
  @Test
  public void searchesByStreetAndLocality()
  {
    final ContactAddress address =
        new ContactAddress("Name", "Home", "6498 131a St, Surrey, BC V3W 7P4, Canada", "6498 131a St", "Surrey");

    assertEquals(List.of(new ContactAddress.SearchQuery("6498 131a Street", "6498 131a Street", false),
                         new ContactAddress.SearchQuery("6498 131a Street Surrey", "6498 131a Street", true)),
                 address.getSearchQueries());
  }

  @Test
  public void fallsBackToFormattedAddressWithoutStructuredFields()
  {
    final ContactAddress address = new ContactAddress("Name", "Home", "868 West 67 Ave, Vancouver", "", "");

    assertEquals(new ContactAddress.SearchQuery("868 West 67th Avenue", "868 West 67th Avenue", false),
                 address.getSearchQueries().get(0));
    assertEquals(new ContactAddress.SearchQuery("868 West 67th Avenue Vancouver", "868 West 67th Avenue", true),
                 address.getSearchQueries().get(1));
    assertEquals(new ContactAddress.SearchQuery(address.address, "868 West 67th Avenue", false),
                 address.getSearchQueries().get(address.getSearchQueries().size() - 1));
  }

  @Test
  public void usesLocalityAndNearbyAddressesForMapResolution()
  {
    final ContactAddress address =
        new ContactAddress("Name", "Home", "6498 131a St, Surrey, BC V3W 7P4, Canada", "6498 131a St", "Surrey");

    assertEquals(new ContactAddress.SearchQuery("6498 131a Street Surrey", "6498 131a Street", true),
                 address.getMapSearchQuery());
  }

  @Test
  public void prefersFormattedAddressWhenStructuredStreetContainsAUnit()
  {
    final ContactAddress address =
        new ContactAddress("Name", "Home", "#15 3495 147A Street, Surrey, BC", "15 3495 147A Street", "Surrey");

    assertEquals(new ContactAddress.SearchQuery("3495 147A Street", "3495 147A Street", false),
                 address.getSearchQueries().get(0));
  }

  @Test
  public void retriesAnAmbiguousLeadingUnitWithoutTheUnit()
  {
    final ContactAddress address =
        new ContactAddress("Name", "Home", "10 578 Corydon Ave, Winnipeg, MB", "10 578 Corydon Ave", "Winnipeg");

    assertEquals(List.of(
                     new ContactAddress.SearchQuery("10 578 Corydon Avenue Winnipeg", "10 578 Corydon Avenue", true),
                     new ContactAddress.SearchQuery("578 Corydon Avenue Winnipeg", "578 Corydon Avenue", true)),
                 address.getMapSearchQueries());
  }

  @Test
  public void ignoresContactNotesThatAreNotAddresses()
  {
    final ContactAddress address = new ContactAddress("Name", "Home", "Same as Mom", "Same as Mom", "");

    assertEquals(List.of(), address.getSearchQueries());
    assertEquals(List.of(), address.getMapSearchQueries());
  }

  @Test
  public void usesStructuredStreetWhenFormattedAddressHasNoSuffix()
  {
    final ContactAddress address =
        new ContactAddress("Name", "Home", "1675 143B Surrey BC Canada", "1675 143B", "Surrey");

    assertEquals(new ContactAddress.SearchQuery("1675 143B", "1675 143B", false),
                 address.getSearchQueries().get(0));
    assertEquals(new ContactAddress.SearchQuery("1675 143B Surrey", "1675 143B", true),
                 address.getMapSearchQuery());
  }
}
