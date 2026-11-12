package app.organicmaps.contacts;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.Executor;
import java.util.function.BooleanSupplier;
import java.util.function.Consumer;

// Main-thread state machine. No observers, durable contact cache, or search-UI integration.
public final class ContactsImporter
{
  public enum Phase
  {
    IDLE,
    READING,
    RESOLVING,
    SAVING,
    DONE,
    CANCELLED,
    ERROR
  }
  public enum Error
  {
    NONE,
    PERMISSION,
    READ,
    SAVE,
    MAPS
  }

  static final class Contact
  {
    final String key;
    final String name;
    final List<ContactAddress> addresses = new ArrayList<>();

    Contact(String key, String name)
    {
      this.key = key;
      this.name = name;
    }

    void addAddress(ContactAddress address)
    {
      if (addresses.stream().noneMatch(item -> item.getAddressKey().equals(address.getAddressKey())))
        addresses.add(address);
    }
  }

  record Location(double latitude, double longitude)
  {
  }
  record Entry(String key, String name, String address, Location location, boolean inContacts, boolean canModify,
               List<String> obsoleteKeys, String sourceKeys)
  {
    Entry(String key, String name, String address, Location location, boolean inContacts, boolean canModify,
          List<String> obsoleteKeys)
    {
      this(key, name, address, location, inContacts, canModify, obsoleteKeys, "");
    }

    Entry(String key, String name, String address, Location location)
    {
      this(key, name, address, location, true, true, List.of());
    }
  }

  public static final class Summary
  {
    public final long categoryId;
    public final long added;
    public final long updated;
    public final long unchanged;
    public final long edited;
    public final long skipped;

    Summary(long[] values)
    {
      if (values == null || values.length != 6)
        throw new IllegalArgumentException("Invalid bookmark import summary");
      categoryId = values[0];
      added = values[1];
      updated = values[2];
      unchanged = values[3];
      edited = values[4];
      skipped = values[5];
    }
  }

  public static final class State
  {
    public final Phase phase;
    public final int processed;
    public final int total;
    public final int unresolved;
    public final Error error;
    public final Summary result;

    State(Phase phase, int processed, int total, int unresolved, Error error, Summary result)
    {
      this.phase = phase;
      this.processed = processed;
      this.total = total;
      this.unresolved = unresolved;
      this.error = error;
      this.result = result;
    }

    public boolean isRunning()
    {
      return phase == Phase.READING || phase == Phase.RESOLVING || phase == Phase.SAVING;
    }
  }

  interface Resolver
  {
    long resolve(ContactAddress address, Consumer<Location> callback);
    void cancel(long id);
  }

  interface Store
  {
    Summary save(List<Entry> entries);
  }

  private final Resolver resolver;
  private final Store store;
  private final Executor main;
  private final BooleanSupplier permitted;
  private final Consumer<State> observer;
  private final Map<String, Location> locations = new HashMap<>();
  private final List<Entry> entries = new ArrayList<>();
  private final Map<String, Entry> protectedEntries = new HashMap<>();
  private final List<Entry> previousEntries = new ArrayList<>();
  private final Map<String, String> streetKeys = new HashMap<>();
  private List<Contact> contacts = List.of();
  private int index;
  private int addressIndex;
  private int unresolved;
  private long generation;
  private long pending;
  private boolean running;

  ContactsImporter(Resolver resolver, Store store, Executor main, BooleanSupplier permitted, Consumer<State> observer)
  {
    this.resolver = resolver;
    this.store = store;
    this.main = main;
    this.permitted = permitted;
    this.observer = observer;
  }

  void start(List<Contact> contacts)
  {
    start(contacts, List.of());
  }

  void start(List<Contact> contacts, List<Entry> previous)
  {
    clear();
    Set<String> ambiguousSources = new java.util.HashSet<>();
    for (Entry entry : previous)
    {
      if (!entry.inContacts() || !entry.canModify())
      {
        protectedEntries.putIfAbsent(entry.key(), entry);
        continue;
      }
      // Older readback used six significant digits. Resolve those inputs once again rather
      // than promoting an already-rounded point into the precise cache.
      for (String sourceKey : decodeSourceKeys(entry.sourceKeys()))
      {
        Location existing = locations.putIfAbsent(sourceKey, entry.location());
        if (existing != null && !existing.equals(entry.location()))
          ambiguousSources.add(sourceKey);
      }
    }
    ambiguousSources.forEach(locations::remove);
    previousEntries.addAll(previous);
    this.contacts = List.copyOf(contacts);
    index = addressIndex = unresolved = 0;
    running = true;
    next();
  }

  void cancel()
  {
    int total = contacts.size();
    clear();
    observer.accept(new State(Phase.CANCELLED, index, total, unresolved, Error.NONE, null));
  }

  private void clear()
  {
    ++generation;
    running = false;
    if (pending != 0)
      resolver.cancel(pending);
    pending = 0;
    contacts = List.of();
    entries.clear();
    locations.clear();
    protectedEntries.clear();
    previousEntries.clear();
    streetKeys.clear();
  }

  private static final double RESIDENCE_RADIUS_METERS = 300.0;

  private String residenceAddress(String address)
  {
    return streetKeys.computeIfAbsent(
        address,
        value
        -> String.join(" ", ContactAddressNormalizer.matchTokens(ContactAddressNormalizer.normalizeStreet(value))));
  }

  private final class ResidenceGroup
  {
    final List<Entry> entries = new ArrayList<>();
    final Set<String> names = new LinkedHashSet<>();

    boolean canAccept(Entry entry)
    {
      if (residenceAddress(entry.address()).isEmpty()
          || !residenceAddress(first().address()).equals(residenceAddress(entry.address())))
        return false;
      return entries.stream().allMatch(
          existing -> distanceMeters(existing.location(), entry.location()) <= RESIDENCE_RADIUS_METERS);
    }

    void add(Entry entry)
    {
      entries.add(entry);
      names.add(entry.name());
    }

    Entry first()
    {
      return entries.get(0);
    }
  }

  // Include house number in the normalized street identity; bound the whole group rather than
  // allowing a chain of nearby points to join unrelated residences.
  private List<Entry> groupByResidence(List<Entry> source)
  {
    List<Entry> ordered = new ArrayList<>(source);
    ordered.sort(java.util.Comparator.comparing(Entry::key));
    List<Entry> result = new ArrayList<>();
    List<ResidenceGroup> groups = new ArrayList<>();
    for (Entry entry : ordered)
    {
      if (protectedEntries.containsKey(entry.key()))
      {
        result.add(entry);
        continue;
      }
      ResidenceGroup target = null;
      for (ResidenceGroup group : groups)
      {
        if (group.canAccept(entry))
        {
          target = group;
          break;
        }
      }
      if (target == null)
      {
        target = new ResidenceGroup();
        groups.add(target);
      }
      target.add(entry);
    }

    Set<String> assignedKeys = new java.util.HashSet<>();
    Set<String> currentKeys = new java.util.HashSet<>();
    source.forEach(entry -> currentKeys.add(entry.key()));
    for (ResidenceGroup group : groups)
    {
      Set<String> groupKeys = new java.util.HashSet<>();
      group.entries.forEach(entry -> groupKeys.add(entry.key()));
      List<String> previousKeys =
          previousEntries.stream()
              .filter(previous -> previous.inContacts() && previous.canModify() && group.canAccept(previous))
              .map(Entry::key)
              .filter(key -> !protectedEntries.containsKey(key) && !assignedKeys.contains(key))
              .filter(key -> !currentKeys.contains(key) || groupKeys.contains(key))
              .distinct()
              .sorted()
              .toList();
      String primaryKey = previousKeys.isEmpty() ? group.first().key() : previousKeys.get(0);
      assignedKeys.addAll(previousKeys);
      assignedKeys.add(primaryKey);
      result.add(new Entry(primaryKey, String.join(", ", group.names), group.first().address(),
                           group.first().location(), true, true,
                           previousKeys.stream().filter(key -> !key.equals(primaryKey)).toList(),
                           "v2:"
                               + String.join(";", group.entries.stream()
                                                      .flatMap(entry -> decodeSourceKeys(entry.sourceKeys()).stream())
                                                      .distinct()
                                                      .sorted()
                                                      .toList())));
    }
    return result;
  }

  private static double distanceMeters(Location first, Location second)
  {
    final double lat1 = Math.toRadians(first.latitude());
    final double lat2 = Math.toRadians(second.latitude());
    final double dLat = lat2 - lat1;
    final double dLon = Math.toRadians(second.longitude() - first.longitude());
    final double sinLat = Math.sin(dLat / 2);
    final double sinLon = Math.sin(dLon / 2);
    final double h = sinLat * sinLat + Math.cos(lat1) * Math.cos(lat2) * sinLon * sinLon;
    return 2 * 6371000 * Math.asin(Math.sqrt(h));
  }

  private void next()
  {
    if (!running)
      return;
    if (!permitted.getAsBoolean())
    {
      finish(Phase.ERROR, Error.PERMISSION, null);
      return;
    }
    while (index < contacts.size())
    {
      observer.accept(new State(Phase.RESOLVING, index, contacts.size(), unresolved, Error.NONE, null));
      Contact contact = contacts.get(index);
      Entry protectedEntry = protectedEntries.get(contact.key);
      if (protectedEntry != null && !contact.addresses.isEmpty())
      {
        // The native store will preserve this user-owned bookmark, regardless of geocoding.
        entries.add(new Entry(contact.key, contact.name, protectedEntry.address(), protectedEntry.location()));
        ++index;
        addressIndex = 0;
        continue;
      }
      while (addressIndex < contact.addresses.size())
      {
        ContactAddress address = contact.addresses.get(addressIndex++);
        if (address.getSearchQueries().isEmpty())
          continue;
        String sourceKey = ContactSourceKey.hash("address-v2:" + address.getAddressKey());
        if (locations.containsKey(sourceKey))
        {
          Location cached = locations.get(sourceKey);
          if (cached == null)
            continue;
          entries.add(resolvedEntry(contact, address, cached, sourceKey));
          break;
        }
        long attempt = ++generation;
        try
        {
          pending = resolver.resolve(address, location -> main.execute(() -> {
            if (!running || attempt != generation)
              return;
            pending = 0;
            Location valid = location;
            if (valid != null
                && (!Double.isFinite(valid.latitude) || !Double.isFinite(valid.longitude)
                    || Math.abs(valid.latitude) > 90 || Math.abs(valid.longitude) > 180))
              valid = null;
            locations.put(sourceKey, valid);
            if (valid != null)
            {
              entries.add(resolvedEntry(contact, address, valid, sourceKey));
              ++index;
              addressIndex = 0;
            }
            next();
          }));
        }
        catch (RuntimeException e)
        {
          finish(Phase.ERROR, Error.SAVE, null);
        }
        return;
      }
      if (entries.isEmpty() || !entries.get(entries.size() - 1).key.equals(contact.key))
        ++unresolved;
      ++index;
      addressIndex = 0;
    }
    if (!permitted.getAsBoolean())
    {
      finish(Phase.ERROR, Error.PERMISSION, null);
      return;
    }
    observer.accept(new State(Phase.SAVING, index, contacts.size(), unresolved, Error.NONE, null));
    try
    {
      Summary result =
          entries.isEmpty() ? new Summary(new long[] {-1, 0, 0, 0, 0, 0}) : store.save(groupByResidence(entries));
      finish(Phase.DONE, Error.NONE, result);
    }
    catch (RuntimeException e)
    {
      finish(Phase.ERROR, Error.SAVE, null);
    }
  }

  private void finish(Phase phase, Error error, Summary result)
  {
    int total = contacts.size();
    clear();
    observer.accept(new State(phase, index, total, unresolved, error, result));
  }

  private static Entry resolvedEntry(Contact contact, ContactAddress address, Location location, String sourceKey)
  {
    return new Entry(contact.key, contact.name, address.address, location, true, true, List.of(), "v2:" + sourceKey);
  }

  private static List<String> decodeSourceKeys(String value)
  {
    if (!value.startsWith("v2:"))
      return List.of();
    List<String> keys = List.of(value.substring(3).split(";", -1));
    return keys.stream().allMatch(key -> key.matches("[0-9a-f]{64}")) ? keys : List.of();
  }
}
