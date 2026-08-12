package app.organicmaps.search;

import androidx.annotation.NonNull;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

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
  final String normalizedName;
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
    normalizedName = name.toLowerCase(Locale.ROOT);
    this.label = label;
    this.address = address;
    this.street = street;
    this.locality = locality;
  }

  @NonNull
  String getNormalizedStreet()
  {
    final String formattedStreet = ContactAddressNormalizer.normalizeStreet(address);
    final String structuredStreet = ContactAddressNormalizer.normalizeStreet(street);
    final boolean useFormattedStreet = ContactAddressNormalizer.looksLikeAddressQuery(formattedStreet) &&
                                       ContactAddressNormalizer.hasRecognizedStreetSuffix(formattedStreet);
    return useFormattedStreet ? formattedStreet : structuredStreet;
  }

  @NonNull
  List<SearchQuery> getSearchQueries()
  {
    final String normalizedStreet = getNormalizedStreet();
    final List<SearchQuery> queries = new ArrayList<>();
    if (ContactAddressNormalizer.looksLikeAddressQuery(normalizedStreet))
    {
      addQuery(queries, new SearchQuery(normalizedStreet, normalizedStreet, false));
      if (!locality.isEmpty())
        addQuery(queries,
                 new SearchQuery(normalizedStreet + " " + ContactAddressNormalizer.normalizeLocality(locality),
                                 normalizedStreet, true));

      final String withoutBareUnit = ContactAddressNormalizer.possibleBareUnitStreet(normalizedStreet);
      if (!withoutBareUnit.isEmpty())
      {
        addQuery(queries, new SearchQuery(withoutBareUnit, withoutBareUnit, false));
        if (!locality.isEmpty())
          addQuery(queries,
                   new SearchQuery(withoutBareUnit + " " + ContactAddressNormalizer.normalizeLocality(locality),
                                   withoutBareUnit, true));
      }
    }

    if (!address.isEmpty())
    {
      final String normalizedAddress = ContactAddressNormalizer.normalizeAddressQuery(address);
      if (ContactAddressNormalizer.looksLikeAddressQuery(normalizedAddress))
        addQuery(queries, new SearchQuery(normalizedAddress, normalizedStreet, true));
      if (street.isEmpty() && !normalizedAddress.equals(address) &&
          ContactAddressNormalizer.looksLikeAddressQuery(address))
        addQuery(queries, new SearchQuery(address, normalizedStreet, false));
    }
    return queries;
  }

  private static void addQuery(@NonNull List<SearchQuery> queries, @NonNull SearchQuery candidate)
  {
    if (queries.stream().noneMatch(query -> query.query.equalsIgnoreCase(candidate.query)))
      queries.add(candidate);
  }

  @NonNull
  SearchQuery getMapSearchQuery()
  {
    final List<SearchQuery> queries = getSearchQueries();
    for (SearchQuery query : queries)
    {
      if (query.allowNearbyHouseNumbers)
        return query;
    }
    return queries.isEmpty() ? new SearchQuery(address, "", true) : queries.get(0);
  }

  @NonNull
  List<SearchQuery> getMapSearchQueries()
  {
    final List<SearchQuery> queries = getSearchQueries();
    final List<SearchQuery> mapQueries = queries.stream().filter(query -> query.allowNearbyHouseNumbers).toList();
    if (!mapQueries.isEmpty())
      return mapQueries;
    return queries.isEmpty() ? List.of() : List.of(queries.get(0));
  }
}
