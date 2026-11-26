package app.organicmaps.contacts;

import static org.junit.Assert.*;

import java.util.List;
import org.junit.Test;

public class ContactsMapAvailabilityTest
{
  private final List<String> regions = List.of("Canada_British Columbia_Vancouver", "Canada_Manitoba_Winnipeg");

  @Test
  public void skipsOnlyExplicitKnownMissingRegions()
  {
    assertFalse(ContactsMapAvailability.mightHaveMap("Alberta", "Canada", regions));
    assertFalse(ContactsMapAvailability.mightHaveMap("Washington", "USA", regions));
    assertTrue(ContactsMapAvailability.mightHaveMap("British Columbia", "Canada", regions));
    assertTrue(ContactsMapAvailability.mightHaveMap("Manitoba", "Canada", regions));
  }

  @Test
  public void unknownContextAndStorageStillReachTheResolver()
  {
    assertTrue(ContactsMapAvailability.mightHaveMap("", "", regions));
    assertTrue(ContactsMapAvailability.mightHaveMap("", "Germany", regions));
    assertTrue(ContactsMapAvailability.mightHaveMap("Washington", "United Kingdom", regions));
    assertTrue(ContactsMapAvailability.mightHaveMap("Alberta", "Canada", List.of()));
  }
}
