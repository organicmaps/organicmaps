package app.organicmaps.search;

import androidx.annotation.NonNull;
import app.organicmaps.sdk.search.SearchResult;
import java.text.Normalizer;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Locale;
import java.util.Set;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

final class ContactAddressResultMatcher
{
  private static final Pattern ORDINAL = Pattern.compile("(\\d+)(?:st|nd|rd|th)");

  private ContactAddressResultMatcher() {}

  static boolean matches(@NonNull ContactAddress address, @NonNull SearchResult result)
  {
    final String contactStreet = address.street.isEmpty() ? firstAddressLine(address.address) : address.street;
    return matches(address, ContactAddressQueryNormalizer.normalizeStreet(contactStreet), result);
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

    final List<String> expectedTokens = tokens(expectedStreet);
    final List<String> resultTokens = tokens(resultAddress);
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
    final long difference = expectedNumber > resultNumber ? expectedNumber - resultNumber : resultNumber - expectedNumber;
    return difference > Integer.MAX_VALUE ? Integer.MAX_VALUE : (int) difference;
  }

  static boolean matches(@NonNull String contactStreet, @NonNull String resultAddress)
  {
    final List<String> streetTokens = tokens(contactStreet);
    if (streetTokens.size() < 2 || !isHouseNumber(streetTokens.get(0)))
      return false;

    final Set<String> resultTokens = new HashSet<>(tokens(resultAddress));
    for (String token : streetTokens)
    {
      if (!resultTokens.contains(token))
        return false;
    }
    return true;
  }

  private static boolean containsAllTokens(@NonNull String value, @NonNull String expected)
  {
    return new HashSet<>(tokens(value)).containsAll(tokens(expected));
  }

  @NonNull
  private static String firstAddressLine(@NonNull String address)
  {
    final int separator = address.indexOf(',');
    return separator < 0 ? address : address.substring(0, separator);
  }

  @NonNull
  private static List<String> tokens(@NonNull String value)
  {
    final String ascii = Normalizer.normalize(value, Normalizer.Form.NFD).replaceAll("\\p{M}+", "");
    final String[] rawTokens = ascii.toLowerCase(Locale.ROOT).split("[^a-z0-9]+");
    final List<String> tokens = new ArrayList<>();
    for (String token : rawTokens)
    {
      if (!token.isEmpty())
        tokens.add(canonicalize(token));
    }
    return tokens;
  }

  @NonNull
  private static String canonicalize(@NonNull String token)
  {
    final Matcher ordinal = ORDINAL.matcher(token);
    if (ordinal.matches())
      return ordinal.group(1);
    return switch (token)
    {
      case "n" -> "north";
      case "s" -> "south";
      case "e" -> "east";
      case "w" -> "west";
      case "st" -> "street";
      case "ave" -> "avenue";
      case "rd" -> "road";
      case "blvd" -> "boulevard";
      case "dr" -> "drive";
      case "ln" -> "lane";
      case "ct" -> "court";
      case "cres" -> "crescent";
      case "hwy" -> "highway";
      case "pkwy" -> "parkway";
      case "pl" -> "place";
      case "ter", "terr" -> "terrace";
      case "trl" -> "trail";
      default -> token;
    };
  }

  private static boolean isHouseNumber(@NonNull String token)
  {
    return token.matches("\\d+[a-z]?");
  }
}
