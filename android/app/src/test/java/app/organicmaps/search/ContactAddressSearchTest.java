package app.organicmaps.search;

import static org.junit.Assert.assertEquals;

import java.util.ArrayList;
import java.util.List;
import java.util.Locale;
import org.junit.Test;

public class ContactAddressSearchTest
{
  @Test
  public void matchesNamesCaseInsensitivelyAndLimitsDistinctNames()
  {
    final List<ContactAddress> addresses =
        List.of(address("Morgan Alex", "Home"), address("Alex Morgan", "Work"), address("Alex Morgan", "Home"),
                address("alex Zhang", "Home"), address("Taylor Smith", "Home"));

    final List<ContactAddress> matches = ContactAddressSearch.findMatches(addresses, "alex");

    assertEquals(3, matches.size());
    assertEquals("Alex Morgan", matches.get(0).name);
    assertEquals("Home", matches.get(0).label);
    assertEquals("Alex Morgan", matches.get(1).name);
    assertEquals("Work", matches.get(1).label);
    assertEquals("alex Zhang", matches.get(2).name);
  }

  @Test
  public void limitsResults()
  {
    final List<ContactAddress> addresses = new ArrayList<>();
    for (int i = 0; i < 25; ++i)
      addresses.add(address("Alex", String.format(Locale.ROOT, "Address %02d", i)));

    final List<ContactAddress> matches = ContactAddressSearch.findMatches(addresses, "alex");

    assertEquals(20, matches.size());
    assertEquals("Address 00", matches.get(0).label);
    assertEquals("Address 19", matches.get(19).label);
  }

  private static ContactAddress address(String name, String label)
  {
    return new ContactAddress(name, label, "123 Main Street, Vancouver", "123 Main Street", "Vancouver");
  }
}
