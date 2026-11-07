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
  public void matchesOnlyNameWordPrefixes()
  {
    final List<ContactAddress> addresses = List.of(address("Thomas Bergan", "Home"));
    assertEquals(List.of(), ContactAddressSearch.findMatches(addresses, "hom"));
    assertEquals(addresses, ContactAddressSearch.findMatches(addresses, "tho"));
    assertEquals(addresses, ContactAddressSearch.findMatches(addresses, "ber"));
    assertEquals(addresses, ContactAddressSearch.findMatches(addresses, "th be"));
    assertEquals(addresses, ContactAddressSearch.findMatches(addresses, "ber tho"));
    assertEquals(List.of(), ContactAddressSearch.findMatches(addresses, "tho smith"));
  }

  @Test
  public void requiresThreeLettersUnlessShortNameMatchesExactly()
  {
    final ContactAddress li = address("Li Smith", "Home");
    final ContactAddress jo = address("Jo Jones", "Home");
    final List<ContactAddress> addresses = List.of(li, jo, address("Liam Jonathan", "Home"));
    assertEquals(List.of(), ContactAddressSearch.findMatches(addresses, "l"));
    assertEquals(List.of(li), ContactAddressSearch.findMatches(addresses, "li"));
    assertEquals(List.of(jo), ContactAddressSearch.findMatches(addresses, "jo"));
    assertEquals(List.of(), ContactAddressSearch.findMatches(addresses, "l j"));
    assertEquals(List.of(), ContactAddressSearch.findMatches(addresses, "l..."));
    assertEquals(List.of(), ContactAddressSearch.findMatches(addresses, "123 #"));
    assertEquals(List.of(), ContactAddressSearch.findMatches(addresses, "  "));
  }

  @Test
  public void doesNotTreatAddressNumbersAsNameSeparators()
  {
    final List<ContactAddress> addresses = List.of(address("Anna Stagliano", "Home"), address("Alex Morgan", "Home"));
    for (String query : List.of("6498 131a st", "6492 131a st, surrey", "131a st", "Alex 123", "Alex \uD835\uDFD4",
                                "\u0666\u0664\u0669\u0668 a st", "\uFF16\uFF14\uFF19\uFF18 a st", "\uD835\uDFD4 a st"))
      assertEquals(query, List.of(), ContactAddressSearch.findMatches(addresses, query));
    assertEquals(List.of(addresses.get(0)), ContactAddressSearch.findMatches(addresses, "anna sta"));
    assertEquals(List.of(addresses.get(1)), ContactAddressSearch.findMatches(addresses, "alex"));
  }

  @Test
  public void countsUnicodeLettersRatherThanUtf16Units()
  {
    final String letter = "\uD801\uDC00";
    final List<ContactAddress> addresses = List.of(address(letter + letter + letter, "Home"));
    assertEquals(List.of(), ContactAddressSearch.findMatches(addresses, letter + letter));
    assertEquals(addresses, ContactAddressSearch.findMatches(addresses, letter + letter + letter));
  }

  @Test
  public void matchesAccentsMiddleNamesAndCompoundSurnames()
  {
    final ContactAddress contact = address("Jos\u00e9 Andr\u00e9 de la Cruz-Smith", "Home");
    final List<ContactAddress> addresses = List.of(contact);
    assertEquals(addresses, ContactAddressSearch.findMatches(addresses, " JOS "));
    assertEquals(addresses, ContactAddressSearch.findMatches(addresses, "jose\u0301"));
    assertEquals(addresses, ContactAddressSearch.findMatches(addresses, "and"));
    assertEquals(addresses, ContactAddressSearch.findMatches(addresses, "cru smi"));
    assertEquals(List.of(), ContactAddressSearch.findMatches(addresses, "ruz"));
  }

  @Test
  public void keepsAllAddressesOfTwoNamesAcrossBothPrefixPasses()
  {
    final List<ContactAddress> addresses =
        List.of(address("Zoe Alex", "Work"), address("Morgan Alex", "Work"), address("Alex Zane", "Work"),
                address("Morgan Alex", "Home"), address("Alex Zane", "Home"));
    final List<ContactAddress> matches = ContactAddressSearch.findMatches(addresses, "alex");
    assertEquals(List.of("Alex Zane", "Alex Zane", "Morgan Alex", "Morgan Alex"),
                 matches.stream().map(address -> address.name).toList());
    assertEquals(List.of("Home", "Work", "Home", "Work"), matches.stream().map(address -> address.label).toList());
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
