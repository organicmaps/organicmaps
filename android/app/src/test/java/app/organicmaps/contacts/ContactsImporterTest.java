package app.organicmaps.contacts;

import static org.junit.Assert.*;

import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.function.Consumer;
import org.junit.Test;

public class ContactsImporterTest
{
  private final ArrayDeque<Runnable> tasks = new ArrayDeque<>();
  private final List<Consumer<ContactsImporter.Location>> callbacks = new ArrayList<>();
  private final List<Long> cancelled = new ArrayList<>();
  private final List<List<ContactsImporter.Entry>> saved = new ArrayList<>();
  private final List<ContactsImporter.State> states = new ArrayList<>();
  private final AtomicBoolean allowed = new AtomicBoolean(true);
  private final ContactsImporter importer = new ContactsImporter(
      new ContactsImporter.Resolver() {
        public long resolve(ContactAddress address, Consumer<ContactsImporter.Location> callback)
        {
          callbacks.add(callback);
          return callbacks.size();
        }
        public void cancel(long id)
        {
          cancelled.add(id);
        }
      },
      entries
      -> {
        saved.add(entries);
        return new ContactsImporter.Summary(new long[] {42, entries.size(), 0, 0, 0, 0});
      },
      tasks::add, allowed::get, states::add);

  private static ContactsImporter.Contact contact(String key, String... addresses)
  {
    var contact = new ContactsImporter.Contact(key, "Name " + key);
    for (String address : addresses)
      contact.addAddress(new ContactAddress(address, "", ""));
    return contact;
  }

  private void complete(int request, ContactsImporter.Location location)
  {
    callbacks.get(request).accept(location);
    while (!tasks.isEmpty())
      tasks.remove().run();
  }

  private static ContactsImporter.Entry cached(String key, String address, ContactsImporter.Location location)
  {
    String fingerprint =
        "v2:" + ContactSourceKey.hash("address-v2:" + new ContactAddress(address, "", "").getAddressKey());
    return new ContactsImporter.Entry(key, "", address, location, true, true, List.of(), fingerprint);
  }

  private ContactsImporter.State last()
  {
    return states.get(states.size() - 1);
  }

  @Test
  public void refreshesLegacyRoundedLocationsOnceThenReusesPrecisePoint()
  {
    String address = "123 Example Street, Example City";
    var people = List.of(contact("one", address));
    for (String oldFingerprint : List.of("", "v1:" + ContactSourceKey.hash("old")))
    {
      int request = callbacks.size();
      importer.start(people, List.of(new ContactsImporter.Entry("one", "", address,
                                                                new ContactsImporter.Location(49.1235, -122.765), true,
                                                                true, List.of(), oldFingerprint)));
      assertEquals(request + 1, callbacks.size());
      var precise = new ContactsImporter.Location(49.1234567, -122.7654321);
      complete(request, precise);
      var previous = saved.get(saved.size() - 1);
      assertEquals(precise, previous.get(0).location());
      importer.start(people, previous);
      assertEquals(request + 1, callbacks.size());
      assertEquals(precise, saved.get(saved.size() - 1).get(0).location());
    }
  }

  @Test
  public void unknownFingerprintDoesNotFallBackToLegacyAddressCache()
  {
    String address = "123 Example Street, Canada";
    importer.start(List.of(contact("one", address)),
                   List.of(new ContactsImporter.Entry("one", "Name", address, new ContactsImporter.Location(49, -123),
                                                      true, true, List.of(), "v2:unknown")));
    assertEquals(1, callbacks.size());
    complete(0, null);
  }

  @Test
  public void reusesStructuredAddressFingerprintAndInvalidatesChangedCity()
  {
    var person = new ContactsImporter.Contact("one", "Person");
    person.addAddress(new ContactAddress("123 Example Street", "123 Example Street", "First City", "BC", "Canada"));
    importer.start(List.of(person));
    complete(0, new ContactsImporter.Location(49, -123));
    var previous = saved.get(0);
    assertTrue(previous.get(0).sourceKeys().startsWith("v2:"));
    importer.start(List.of(person), previous);
    assertEquals(1, callbacks.size());
    assertEquals(2, saved.size());
    person.addresses.clear();
    person.addAddress(new ContactAddress("123 Example Street", "123 Example Street", "Other City", "BC", "Canada"));
    importer.start(List.of(person), previous);
    assertEquals(2, callbacks.size());
    complete(1, null);
  }

  @Test
  public void retainsAllResidenceMemberFingerprints()
  {
    var first = contact("one", "#10 2453 Example Street, Surrey, BC, Canada");
    var second = contact("two", "2453 Example St. Surrey");
    importer.start(List.of(first, second));
    complete(0, new ContactsImporter.Location(49, -123));
    complete(1, new ContactsImporter.Location(49, -123));
    assertEquals(1, saved.get(0).size());
    assertEquals(2, saved.get(0).get(0).sourceKeys().substring(3).split(";").length);
    importer.start(List.of(first, second), saved.get(0));
    assertEquals(2, callbacks.size());
    assertEquals(1, saved.get(1).size());
  }

  @Test
  public void reusesUnchangedImportedAddressWithoutSearching()
  {
    String address = "123 Main Street, Canada";
    importer.start(List.of(contact("one", address)),
                   List.of(cached("one", address, new ContactsImporter.Location(49, -123))));
    assertTrue(callbacks.isEmpty());
    assertEquals("Name one", saved.get(0).get(0).name());
    assertEquals(ContactsImporter.Phase.DONE, last().phase);
  }

  @Test
  public void reusesSavedAddressForOtherResidentsWithoutSearching()
  {
    String address = "8877 Example Street, Example City";
    importer.start(List.of(contact("first", address), contact("second", address)),
                   List.of(cached("old", address, new ContactsImporter.Location(49.1, -122.6))));
    assertTrue(callbacks.isEmpty());
    assertEquals(1, saved.get(0).size());
    assertEquals("Name first, Name second", saved.get(0).get(0).name());
  }

  @Test
  public void sharesSavedAndNewlyResolvedLocationsWithinOneImport()
  {
    String savedAddress = "123 Main Street, Canada";
    String newAddress = "456 Other Street, Canada";
    var previous = new ContactsImporter.Location(49, -123);
    var resolved = new ContactsImporter.Location(50, -124);
    importer.start(List.of(contact("one", savedAddress), contact("two", newAddress), contact("three", savedAddress),
                           contact("four", newAddress)),
                   List.of(cached("old", savedAddress, previous)));
    assertEquals(1, callbacks.size());
    complete(0, resolved);
    assertEquals(1, callbacks.size());
    assertEquals(2, saved.get(0).size());
    assertTrue(saved.get(0).stream().anyMatch(entry -> entry.location().equals(previous)));
    assertTrue(saved.get(0).stream().anyMatch(entry -> entry.location().equals(resolved)));
    assertEquals(ContactsImporter.Phase.DONE, last().phase);
  }

  @Test
  public void doesNotReuseConflictingSavedAddressLocations()
  {
    String address = "123 Main Street, Canada";
    importer.start(List.of(contact("new", address)),
                   List.of(cached("old1", address, new ContactsImporter.Location(49, -123)),
                           cached("old2", address, new ContactsImporter.Location(50, -124))));
    assertEquals(1, callbacks.size());
    complete(0, null);
  }

  @Test
  public void doesNotGeocodeMovedOrLocallyEditedBookmarks()
  {
    String address = "123 Main Street, Canada";
    importer.start(List.of(contact("moved", address), contact("edited", address)),
                   List.of(new ContactsImporter.Entry("moved", "", address, new ContactsImporter.Location(49, -123),
                                                      false, true, List.of()),
                           new ContactsImporter.Entry("edited", "", address, new ContactsImporter.Location(49, -123),
                                                      true, false, List.of())));
    assertTrue(callbacks.isEmpty());
    assertEquals(2, saved.get(0).size());
    assertTrue(saved.get(0).stream().allMatch(entry -> entry.obsoleteKeys().isEmpty()));
  }

  @Test
  public void sharedAddressCacheDoesNotOverrideStructuredLocality()
  {
    var person = new ContactsImporter.Contact("new", "Person");
    person.addAddress(new ContactAddress("123 Main Street", "123 Main Street", "Other City", "BC", "Canada"));
    importer.start(List.of(person), List.of(new ContactsImporter.Entry("old", "", "123 Main Street",
                                                                       new ContactsImporter.Location(49, -123))));
    assertEquals(1, callbacks.size());
    complete(0, null);
  }

  @Test
  public void changedSourceAddressStillRequiresSearching()
  {
    importer.start(List.of(contact("one", "456 Other Road, Canada")),
                   List.of(new ContactsImporter.Entry("one", "Name", "123 Main Street, Canada",
                                                      new ContactsImporter.Location(49, -123))));
    assertEquals(1, callbacks.size());
    assertTrue(saved.isEmpty());
    complete(0, null);
    assertEquals(1, last().unresolved);
  }

  @Test
  public void resolvesOnceForPeopleSharingAnAddress()
  {
    importer.start(List.of(contact("one", "123 Main Street, Canada"), contact("two", "123 Main Street, Canada")));
    assertTrue(saved.isEmpty());
    complete(0, new ContactsImporter.Location(49, -123));
    assertEquals(1, callbacks.size());
    assertEquals(1, saved.get(0).size());
    assertEquals("Name one, Name two", saved.get(0).get(0).name());
    assertEquals(ContactsImporter.Phase.DONE, last().phase);
    assertEquals(0, last().unresolved);
  }

  @Test
  public void mergesResidentsWithDifferentUnitsAndFormatting()
  {
    importer.start(
        List.of(contact("luke", "#10 2453 163 St, Surrey, BC, Canada"), contact("natalie", "2453 163 St. Surrey")));
    complete(0, new ContactsImporter.Location(49.0465, -122.771));
    complete(1, new ContactsImporter.Location(49.046484, -122.771031));
    assertEquals(1, saved.get(0).size());
    assertEquals("Name luke, Name natalie", saved.get(0).get(0).name());
  }

  @Test
  public void keepsDifferentHouseNumbersOnTheSameStreetSeparate()
  {
    importer.start(List.of(contact("one", "123 Main Street, Canada"), contact("two", "125 Main Street, Canada")));
    complete(0, new ContactsImporter.Location(49, -123));
    complete(1, new ContactsImporter.Location(49.0001, -123.0001));
    assertEquals(2, saved.get(0).size());
  }

  @Test
  public void residenceIdentityIgnoresCaseAndOrdinalFormatting()
  {
    importer.start(
        List.of(contact("luke", "#10 2453 163rd street, Surrey"), contact("natalie", "2453 163 St. Surrey")));
    complete(0, new ContactsImporter.Location(49.0465, -122.771));
    complete(1, new ContactsImporter.Location(49.046484, -122.771031));
    assertEquals(1, saved.get(0).size());
  }

  @Test
  public void retiresOldPerContactBookmarksAtTheSameResidence()
  {
    importer.start(List.of(contact("luke", "#10 2453 163 St, Surrey, BC"), contact("natalie", "2453 163 St. Surrey")),
                   List.of(new ContactsImporter.Entry("oldLuke", "", "#10 2453 163 St, Surrey, BC",
                                                      new ContactsImporter.Location(49.0465, -122.771)),
                           new ContactsImporter.Entry("oldNatalie", "", "2453 163 St. Surrey",
                                                      new ContactsImporter.Location(49.046484, -122.771031))));
    complete(0, new ContactsImporter.Location(49.0465, -122.771));
    complete(1, new ContactsImporter.Location(49.046484, -122.771031));
    assertEquals(1, saved.get(0).size());
    assertEquals("oldLuke", saved.get(0).get(0).key());
    assertEquals(List.of("oldNatalie"), saved.get(0).get(0).obsoleteKeys());
  }

  @Test
  public void mergesCodyAndBevEllisAtWrightStreet()
  {
    var cody = contact("cody", "8877 Wright St, Langley");
    var bev = contact("bev", "Unit 2, 8877 Wright Street, Langley, BC, Canada");
    importer.start(List.of(cody, bev));
    complete(0, new ContactsImporter.Location(49.1, -122.6));
    complete(1, new ContactsImporter.Location(49.1001, -122.6001));
    assertEquals(1, saved.get(0).size());
    assertEquals("Name bev, Name cody", saved.get(0).get(0).name());
  }

  @Test
  public void doesNotRetireMovedOrLocallyEditedResidenceBookmarks()
  {
    String address = "8877 Wright St, Langley";
    var point = new ContactsImporter.Location(49.1, -122.6);
    importer.start(List.of(contact("new", address)),
                   List.of(new ContactsImporter.Entry("moved", "", address, point, false, true, List.of()),
                           new ContactsImporter.Entry("edited", "", address, point, true, false, List.of())));
    complete(0, point);
    assertEquals("new", saved.get(0).get(0).key());
    assertTrue(saved.get(0).get(0).obsoleteKeys().isEmpty());
  }

  @Test
  public void doesNotReuseTheKeyOfAContactWhoMovedToAnotherResidence()
  {
    importer.start(
        List.of(contact("one", "123 Main Street"), contact("two", "456 Other Road")),
        List.of(new ContactsImporter.Entry("two", "", "123 Main Street", new ContactsImporter.Location(49, -123))));
    complete(0, new ContactsImporter.Location(49, -123));
    assertEquals(2, callbacks.size());
    complete(1, new ContactsImporter.Location(50, -124));
    assertEquals("one", saved.get(0).get(0).key());
    assertEquals("two", saved.get(0).get(1).key());
    assertTrue(saved.get(0).get(0).obsoleteKeys().isEmpty());
  }

  @Test
  public void triesOtherAddressesAfterAnUnresolvedPrimary()
  {
    importer.start(List.of(contact("one", "123 Main Street, Canada", "456 Other Road, Canada")));
    complete(0, null);
    assertTrue(saved.isEmpty());
    complete(1, new ContactsImporter.Location(49, -123));
    assertEquals("456 Other Road, Canada", saved.get(0).get(0).address());
    assertEquals(1, saved.get(0).size());
  }

  @Test
  public void cancelDoesNotSavePartialResults()
  {
    importer.start(List.of(contact("one", "123 Main Street, Canada"), contact("two", "456 Other Road, Canada")));
    complete(0, new ContactsImporter.Location(49, -123));
    importer.cancel();
    complete(1, new ContactsImporter.Location(50, -124));
    assertTrue(saved.isEmpty());
    assertEquals(List.of(2L), cancelled);
    assertEquals(ContactsImporter.Phase.CANCELLED, last().phase);
    assertEquals(2, last().total);
  }

  @Test
  public void restartIgnoresAnOlderImportsCallback()
  {
    importer.start(List.of(contact("old", "123 Main Street, Canada")));
    importer.cancel();
    importer.start(List.of(contact("new", "456 Other Road, Canada")));
    complete(0, new ContactsImporter.Location(49, -123));
    assertTrue(saved.isEmpty());
    complete(1, new ContactsImporter.Location(50, -124));
    assertEquals("new", saved.get(0).get(0).key());
  }

  @Test
  public void permissionRevocationDoesNotSaveOrContinue()
  {
    importer.start(List.of(contact("one", "123 Main Street, Canada")));
    allowed.set(false);
    complete(0, new ContactsImporter.Location(49, -123));
    assertTrue(saved.isEmpty());
    assertEquals(ContactsImporter.Error.PERMISSION, last().error);
  }

  @Test
  public void allMissingAddressesDoNotCreateAList()
  {
    importer.start(List.of(contact("one", "123 Main Street, Canada"), contact("two", "123 Main Street, Canada")));
    complete(0, null);
    assertEquals(1, callbacks.size());
    assertTrue(saved.isEmpty());
    assertEquals(2, last().unresolved);
    assertEquals(-1, last().result.categoryId);
  }

  @Test
  public void skipsNonAddressesWithoutCallingTheResolver()
  {
    importer.start(List.of(contact("one", "same as my parents")));
    assertTrue(callbacks.isEmpty());
    assertEquals(1, last().unresolved);
  }

  @Test
  public void rejectsNonFiniteCoordinates()
  {
    importer.start(List.of(contact("one", "123 Main Street, Canada")));
    complete(0, new ContactsImporter.Location(Double.NaN, -123));
    assertTrue(saved.isEmpty());
    assertEquals(1, last().unresolved);
  }

  @Test
  public void missingAddressCacheDoesNotSurviveTheImport()
  {
    var contacts = List.of(contact("one", "123 Main Street, Canada"));
    importer.start(contacts);
    complete(0, null);
    importer.start(contacts);
    assertEquals(2, callbacks.size());
    complete(1, new ContactsImporter.Location(49, -123));
    assertEquals(1, saved.size());
  }

  @Test
  public void providerKeysAreHashedAndDeterministic()
  {
    String key = ContactsReader.hashKey("lookup:personal-provider-key");
    assertEquals(64, key.length());
    assertTrue(key.matches("[0-9a-f]{64}"));
    assertEquals(key, ContactsReader.hashKey("lookup:personal-provider-key"));
    assertNotEquals(key, ContactsReader.hashKey("lookup:different"));
  }

  @Test
  public void contactDropsDuplicateAddressRows()
  {
    assertEquals(1, contact("one", "123 Main Street, Canada", "123 Main Street, Canada").addresses.size());
  }
}
