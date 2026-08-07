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
    assertEquals(new ContactAddress.SearchQuery(address.address, "868 West 67th Avenue", false),
                 address.getSearchQueries().get(address.getSearchQueries().size() - 1));
  }
}
