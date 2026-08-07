package app.organicmaps.search;

import androidx.annotation.NonNull;
import java.util.ArrayList;
import java.util.List;

final class ContactAddress
{
  static final class SearchQuery
  {
    @NonNull
    final String query;
    @NonNull
    final String expectedStreet;
    final boolean allowNearbyHouseNumbers;

    SearchQuery(@NonNull String query, @NonNull String expectedStreet, boolean allowNearbyHouseNumbers)
    {
      this.query = query;
      this.expectedStreet = expectedStreet;
      this.allowNearbyHouseNumbers = allowNearbyHouseNumbers;
    }

    @Override
    public boolean equals(Object object)
    {
      return object instanceof SearchQuery other && query.equals(other.query) &&
             expectedStreet.equals(other.expectedStreet) && allowNearbyHouseNumbers == other.allowNearbyHouseNumbers;
    }

    @Override
    public int hashCode()
    {
      return 31 * (31 * query.hashCode() + expectedStreet.hashCode()) + Boolean.hashCode(allowNearbyHouseNumbers);
    }

    @Override
    public String toString()
    {
      return query;
    }
  }

  @NonNull
  final String name;
  @NonNull
  final String label;
  @NonNull
  final String address;
  @NonNull
  final String street;
  @NonNull
  final String locality;

  ContactAddress(@NonNull String name, @NonNull String label, @NonNull String address, @NonNull String street,
                 @NonNull String locality)
  {
    this.name = name;
    this.label = label;
    this.address = address;
    this.street = street;
    this.locality = locality;
  }

  @NonNull
  List<SearchQuery> getSearchQueries()
  {
    final String normalizedStreet = ContactAddressQueryNormalizer.normalizeStreet(street.isEmpty() ? address : street);
    final List<SearchQuery> queries = new ArrayList<>();
    if (!normalizedStreet.isEmpty())
    {
      queries.add(new SearchQuery(normalizedStreet, normalizedStreet, false));
      if (!locality.isEmpty())
        queries.add(new SearchQuery(normalizedStreet + " " + locality, normalizedStreet, true));
      else
        queries.add(new SearchQuery(normalizedStreet, normalizedStreet, true));
    }
    if (street.isEmpty() && !address.isEmpty() && queries.stream().noneMatch(query -> query.query.equals(address)))
      queries.add(new SearchQuery(address, normalizedStreet, false));
    return queries;
  }
}
