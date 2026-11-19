package app.organicmaps.contacts;

import app.organicmaps.sdk.downloader.CountryItem;
import app.organicmaps.sdk.downloader.MapManager;
import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;

/** Main-thread snapshot of usable map regions, not a contact-location cache. */
final class ContactsMapAvailability
{
  private List<String> regions = List.of();
  private int count = -1;

  void invalidate()
  {
    count = -1;
  }

  static boolean mightHaveMap(String region, String country, List<String> regions)
  {
    if (!ContactAddressNormalizer.usesNorthAmericanFormatting(country))
      return true;
    // Only structured region/country fields may exclude maps. A city named Washington, for
    // example, must not be mistaken for the US state when it occurs in a formatted address.
    String context = ContactAddressNormalizer.normalizeContext(region, country) + " "
                   + ContactAddressNormalizer.normalizeCountry(country);
    // An incomplete/unknown storage snapshot must not suppress a potentially valid search.
    return regions.isEmpty()
 || regions.stream().anyMatch(map -> ContactAddressNormalizer.matchesMapRegion(context, map));
  }

  boolean mightHaveMap(ContactAddress address)
  {
    String country = ContactAddressNormalizer.formattingCountry(address.country, address.address);
    if (count != MapManager.nativeGetDownloadedCount())
      refresh();
    if (mightHaveMap(address.region, country, regions))
      return true;
    // Storage can change during import, including replacement of maps without a count change.
    refresh();
    return mightHaveMap(address.region, country, regions);
  }

  private void refresh()
  {
    count = MapManager.nativeGetDownloadedCount();
    var items = new ArrayList<CountryItem>();
    MapManager.nativeListItems(null, 0, 0, false, true, items);
    var pending = new ArrayDeque<>(items);
    var seen = new HashSet<String>();
    var leaves = new ArrayList<String>();
    while (!pending.isEmpty())
    {
      var item = pending.removeFirst();
      if (!seen.add(item.id))
        continue;
      if (!item.isExpandable())
      {
        if (item.present)
          leaves.add(item.id);
      }
      else
      {
        items.clear();
        MapManager.nativeListItems(item.id, 0, 0, false, true, items);
        pending.addAll(items);
      }
    }
    regions = leaves.size() == count ? List.copyOf(leaves) : List.of();
  }
}
