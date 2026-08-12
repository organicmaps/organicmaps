package app.organicmaps.search;

import androidx.annotation.NonNull;
import app.organicmaps.sdk.search.SearchResult;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

final class ContactAddressResultMatcher
{
  private ContactAddressResultMatcher() {}

  static boolean matches(@NonNull ContactAddress address, @NonNull SearchResult result)
  {
    return matches(address, address.getNormalizedStreet(), result);
  }

  static boolean matches(@NonNull ContactAddress address, @NonNull String expectedStreet,
                         @NonNull SearchResult result)
  {
    return houseNumberDifference(address, expectedStreet, result) == 0;
  }

  static int houseNumberDifference(@NonNull ContactAddress address, @NonNull String expectedStreet,
                                   @NonNull SearchResult result)
  {
    if (result.type != SearchResult.TYPE_RESULT || result.description == null)
      return Integer.MAX_VALUE;
    final String resultAddress = result.name + " " + result.description.region;
    if (!address.locality.isEmpty() && !containsAllTokens(resultAddress, address.locality))
      return Integer.MAX_VALUE;
    if (matches(expectedStreet, resultAddress))
      return 0;

    final List<String> expectedTokens = ContactAddressNormalizer.matchTokens(expectedStreet);
    final List<String> resultTokens = ContactAddressNormalizer.matchTokens(resultAddress);
    if (expectedTokens.size() < 2 || resultTokens.isEmpty() || !expectedTokens.get(0).matches("\\d+") ||
        !resultTokens.get(0).matches("\\d+") ||
        !new HashSet<>(resultTokens).containsAll(expectedTokens.subList(1, expectedTokens.size())))
      return Integer.MAX_VALUE;

    final long expectedNumber;
    final long resultNumber;
    try
    {
      expectedNumber = Long.parseLong(expectedTokens.get(0));
      resultNumber = Long.parseLong(resultTokens.get(0));
    }
    catch (NumberFormatException ignored)
    {
      return Integer.MAX_VALUE;
    }
    if ((expectedNumber & 1) != (resultNumber & 1))
      return Integer.MAX_VALUE;
    final long difference =
        expectedNumber > resultNumber ? expectedNumber - resultNumber : resultNumber - expectedNumber;
    return difference > Integer.MAX_VALUE ? Integer.MAX_VALUE : (int) difference;
  }

  static boolean matches(@NonNull String contactStreet, @NonNull String resultAddress)
  {
    final List<String> streetTokens = ContactAddressNormalizer.matchTokens(contactStreet);
    if (streetTokens.size() < 2 || !isHouseNumber(streetTokens.get(0)))
      return false;

    final Set<String> resultTokens = new HashSet<>(ContactAddressNormalizer.matchTokens(resultAddress));
    for (String token : streetTokens)
    {
      if (!resultTokens.contains(token))
        return false;
    }
    return true;
  }

  private static boolean containsAllTokens(@NonNull String value, @NonNull String expected)
  {
    return new HashSet<>(ContactAddressNormalizer.matchTokens(value))
        .containsAll(ContactAddressNormalizer.matchTokens(expected));
  }

  private static boolean isHouseNumber(@NonNull String token)
  {
    return token.matches("\\d+[a-z]?");
  }
}
