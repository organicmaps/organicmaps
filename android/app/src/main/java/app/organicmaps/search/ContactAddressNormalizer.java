package app.organicmaps.search;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import java.text.Normalizer;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.util.stream.Collectors;

final class ContactAddressNormalizer
{
  private static final Pattern UNIT_PREFIX =
      Pattern.compile("(?i)^\\s*(?:(?:unit|suite|apt\\.?|apartment|cpo)\\s*#?\\s*|#)\\w+[,.\\s]+(?=\\d+\\s)");
  private static final Pattern BASEMENT_PREFIX = Pattern.compile("(?i)^\\s*basement\\s+(?=\\d+\\s)");
  private static final Pattern LEADING_NUMBER_SEPARATOR =
      Pattern.compile("^\\s*(\\d+[A-Za-z]?)\\s*[-/]\\s*(\\d+[A-Za-z]?)\\s+(.+)$");
  private static final Pattern TRAILING_UNIT =
      Pattern.compile("(?i)\\s+(?:(?:unit|suite|apt\\.?|apartment)\\s*|#)\\w+\\s*$");
  private static final Pattern CANADIAN_POSTAL_CODE =
      Pattern.compile("(?i)\\b[ABCEGHJ-NPRSTVXY]\\d[ABCEGHJ-NPRSTV-Z]\\s*\\d[ABCEGHJ-NPRSTV-Z]\\d\\b");
  private static final Pattern US_ZIP_CODE =
      Pattern.compile("(?i)\\s+\\d{5}(?:-\\d{4})?(?=\\s*(?:USA|U\\.?S\\.?A\\.?|United States(?: of America)?)?\\s*$)");
  private static final Pattern ORDINAL = Pattern.compile("(\\d+)(?:st|nd|rd|th)", Pattern.CASE_INSENSITIVE);
  private static final Pattern ATTACHED_SUFFIX =
      Pattern.compile("(?i)^(\\d+[A-Za-z]?)(st|ave|av|rd|blvd|dr|ln|ct|cres|cr|cir|pl|ter|terr|trl|wy)$");
  private static final Pattern ATTACHED_DIRECTIONAL_NUMBER = Pattern.compile("(?i)^([NSEW])(\\d+)(?:st|nd|rd|th)?$");
  private static final Pattern ADDRESS_START = Pattern.compile("(?i)\\b\\d+[A-Za-z]?\\s+(?=\\S)");
  private static final Pattern BARE_UNIT_STREET = Pattern.compile("^(\\d+)\\s+(\\d+[A-Za-z]?)\\s+(.+)$");

  private ContactAddressNormalizer() {}

  private static final String[][] REGION_NAMES = {{"AB", "Alberta"},
                                                  {"BC", "British Columbia"},
                                                  {"MB", "Manitoba"},
                                                  {"NB", "New Brunswick"},
                                                  {"NL", "Newfoundland and Labrador"},
                                                  {"NS", "Nova Scotia"},
                                                  {"NT", "Northwest Territories"},
                                                  {"NU", "Nunavut"},
                                                  {"ON", "Ontario"},
                                                  {"PE", "Prince Edward Island"},
                                                  {"QC", "Quebec"},
                                                  {"SK", "Saskatchewan"},
                                                  {"YT", "Yukon"},
                                                  {"AL", "Alabama"},
                                                  {"AK", "Alaska"},
                                                  {"AZ", "Arizona"},
                                                  {"AR", "Arkansas"},
                                                  {"CA", "California"},
                                                  {"CO", "Colorado"},
                                                  {"CT", "Connecticut"},
                                                  {"DE", "Delaware"},
                                                  {"DC", "District of Columbia"},
                                                  {"FL", "Florida"},
                                                  {"GA", "Georgia"},
                                                  {"HI", "Hawaii"},
                                                  {"ID", "Idaho"},
                                                  {"IL", "Illinois"},
                                                  {"IN", "Indiana"},
                                                  {"IA", "Iowa"},
                                                  {"KS", "Kansas"},
                                                  {"KY", "Kentucky"},
                                                  {"LA", "Louisiana"},
                                                  {"ME", "Maine"},
                                                  {"MD", "Maryland"},
                                                  {"MA", "Massachusetts"},
                                                  {"MI", "Michigan"},
                                                  {"MN", "Minnesota"},
                                                  {"MS", "Mississippi"},
                                                  {"MO", "Missouri"},
                                                  {"MT", "Montana"},
                                                  {"NE", "Nebraska"},
                                                  {"NV", "Nevada"},
                                                  {"NH", "New Hampshire"},
                                                  {"NJ", "New Jersey"},
                                                  {"NM", "New Mexico"},
                                                  {"NY", "New York"},
                                                  {"NC", "North Carolina"},
                                                  {"ND", "North Dakota"},
                                                  {"OH", "Ohio"},
                                                  {"OK", "Oklahoma"},
                                                  {"OR", "Oregon"},
                                                  {"PA", "Pennsylvania"},
                                                  {"RI", "Rhode Island"},
                                                  {"SC", "South Carolina"},
                                                  {"SD", "South Dakota"},
                                                  {"TN", "Tennessee"},
                                                  {"TX", "Texas"},
                                                  {"UT", "Utah"},
                                                  {"VT", "Vermont"},
                                                  {"VA", "Virginia"},
                                                  {"WA", "Washington"},
                                                  {"WV", "West Virginia"},
                                                  {"WI", "Wisconsin"},
                                                  {"WY", "Wyoming"}};

  private static final Map<String, String> REGIONS =
      Arrays.stream(REGION_NAMES).collect(Collectors.toMap(region -> region[0], region -> region[1]));
  private static final Pattern REGION_CODE = Pattern.compile("\\b(?:" + String.join("|", REGIONS.keySet()) + ")\\b");
  private static final List<List<String>> REGION_TOKENS =
      REGIONS.values().stream().map(ContactAddressNormalizer::matchTokens).toList();

  @NonNull
  static String normalizeCountry(@NonNull String country)
  {
    return country.matches("(?i)[a-z]{2}")
      ? new Locale.Builder().setRegion(country.toUpperCase(Locale.ROOT)).build().getDisplayCountry(Locale.ENGLISH)
      : country;
  }

  @NonNull
  static String format(@Nullable String formattedAddress, @Nullable String... addressParts)
  {
    if (formattedAddress != null && !formattedAddress.isEmpty())
      return joinNonEmpty(formattedAddress.split("[\\r\\n]+"));
    return joinNonEmpty(addressParts);
  }

  @NonNull
  static String normalizeStreet(@NonNull String value)
  {
    final List<String> tokens = normalizeTokens(prepareAddress(value));
    final int streetEnd = findStreetEnd(tokens);
    if (streetEnd >= 0)
      return String.join(" ", tokens.subList(0, streetEnd + 1));
    return String.join(" ", tokens);
  }

  static boolean looksLikeAddressQuery(@NonNull String value)
  {
    final String prepared = prepareAddress(value);
    return looksLikeStructuredStreet(prepared) && hasRecognizedStreetSuffix(prepared);
  }

  static boolean looksLikeStructuredStreet(@NonNull String value)
  {
    return value.matches("\\d+[\\p{L}]?\\s+\\S.*");
  }

  static boolean hasRecognizedStreetSuffix(@NonNull String value)
  {
    return findStreetEnd(normalizeTokens(value)) >= 0;
  }

  @NonNull
  static List<String> matchTokens(@NonNull String value)
  {
    final String ascii = Normalizer.normalize(value, Normalizer.Form.NFD).replaceAll("\\p{M}+", "");
    final String[] rawTokens = ascii.split("[^\\p{L}\\p{N}]+");
    final List<String> tokens = new ArrayList<>(rawTokens.length);
    for (String token : rawTokens)
    {
      if (token.isEmpty())
        continue;
      final Matcher ordinal = ORDINAL.matcher(token);
      if (ordinal.matches())
        tokens.add(ordinal.group(1));
      else
        tokens.add(expandSuffix(expandDirection(token)).toLowerCase(Locale.ROOT));
    }
    return tokens;
  }

  @NonNull
  static String possibleBareUnitStreet(@NonNull String value)
  {
    final Matcher matcher = BARE_UNIT_STREET.matcher(value);
    if (!matcher.matches() || leadingNumber(matcher.group(1)) >= leadingNumber(matcher.group(2)))
      return "";
    return matcher.group(2) + " " + matcher.group(3);
  }

  @NonNull
  static String normalizeContext(@NonNull String value)
  {
    String context = CANADIAN_POSTAL_CODE.matcher(value.replaceAll("[,;\\r\\n]+", " ")).replaceAll(" ");
    context = US_ZIP_CODE.matcher(context).replaceAll(" ");
    final Matcher codes = REGION_CODE.matcher(context);
    final StringBuffer expanded = new StringBuffer();
    while (codes.find())
      codes.appendReplacement(expanded, REGIONS.get(codes.group()));
    codes.appendTail(expanded);
    context = expanded.toString();
    return context.replaceAll("[,;\\r\\n]+", " ").replaceAll("\\s+", " ").trim();
  }

  static boolean matchesMapRegion(@NonNull String context, @NonNull String mapRegion)
  {
    return matchesMapRegion(matchTokens(context), matchTokens(mapRegion));
  }

  static boolean matchesMapRegion(@NonNull List<String> addressTokens, @NonNull List<String> mapTokens)
  {
    if (mapTokens.isEmpty())
      return false;
    boolean hasRegion = false;
    for (List<String> regionTokens : REGION_TOKENS)
    {
      if (addressTokens.containsAll(regionTokens))
      {
        hasRegion = true;
        if (mapTokens.containsAll(regionTokens))
          return true;
      }
    }
    if (hasRegion)
      return false;
    if (addressTokens.contains("canada"))
      return mapTokens.contains("canada");
    if (addressTokens.contains("usa") || addressTokens.contains("us") || addressTokens.contains("united"))
      return mapTokens.contains("us") || mapTokens.contains("usa") || mapTokens.contains("united");
    // No geographical clue: the native resolver still limits the search and result to the viewport.
    return true;
  }

  @NonNull
  private static String prepareAddress(@NonNull String value)
  {
    String address = value.trim();
    while (address.length() >= 2 && isMatchingWrapper(address.charAt(0), address.charAt(address.length() - 1)))
      address = address.substring(1, address.length() - 1).trim();
    address = address.replaceAll("[\\r\\n]+", " ").replace('|', ' ');
    address = UNIT_PREFIX.matcher(address).replaceFirst("");
    address = BASEMENT_PREFIX.matcher(address).replaceFirst("");
    address = normalizeLeadingNumberSeparator(address);
    address = TRAILING_UNIT.matcher(address).replaceFirst("");

    if (!address.matches("^\\d+[A-Za-z]?(?:\\s|,)+\\S.*"))
    {
      final String extracted = extractAddressFromText(address);
      if (!extracted.isEmpty())
        address = extracted;
    }
    return address.replaceAll("\\s+", " ").trim();
  }

  @NonNull
  private static String extractAddressFromText(@NonNull String value)
  {
    final Matcher start = ADDRESS_START.matcher(value);
    while (start.find())
    {
      final String candidate = value.substring(start.start());
      if (findStreetEnd(normalizeTokens(candidate)) >= 0)
        return candidate;
    }
    return "";
  }

  @NonNull
  private static String joinNonEmpty(@Nullable String... parts)
  {
    final List<String> normalized = new ArrayList<>();
    if (parts == null)
      return "";
    for (String part : parts)
    {
      if (part == null)
        continue;
      final String value = part.trim();
      if (!value.isEmpty() && !normalized.contains(value))
        normalized.add(value);
    }
    return String.join(", ", normalized);
  }

  @NonNull
  private static String normalizeLeadingNumberSeparator(@NonNull String value)
  {
    final Matcher matcher = LEADING_NUMBER_SEPARATOR.matcher(value);
    if (!matcher.matches())
      return value;
    final long first = leadingNumber(matcher.group(1));
    final long second = leadingNumber(matcher.group(2));
    if (first < second)
      return matcher.group(2) + " " + matcher.group(3);
    return matcher.group(1) + " " + matcher.group(2) + " " + matcher.group(3);
  }

  private static long leadingNumber(@NonNull String token)
  {
    try
    {
      return Long.parseLong(token.replaceFirst("[A-Za-z]+$", ""));
    }
    catch (NumberFormatException ignored)
    {
      return Long.MAX_VALUE;
    }
  }

  @NonNull
  private static List<String> normalizeTokens(@NonNull String value)
  {
    final String cleaned =
        value.replaceAll("[\\[\\]{}()]", " ").replaceAll("[.,;:|!]", " ").replaceAll("\\s+", " ").trim();
    if (cleaned.isEmpty())
      return new ArrayList<>();

    final String[] rawTokens = cleaned.split(" ");
    final List<String> tokens = new ArrayList<>(rawTokens.length + 2);
    for (int i = 0; i < rawTokens.length; ++i)
    {
      final Matcher directional = ATTACHED_DIRECTIONAL_NUMBER.matcher(rawTokens[i]);
      if (directional.matches())
      {
        tokens.add(expandDirection(directional.group(1)));
        tokens.add(toOrdinal(directional.group(2)));
        continue;
      }

      final Matcher suffix = ATTACHED_SUFFIX.matcher(rawTokens[i]);
      if (suffix.matches())
      {
        tokens.add(suffix.group(1));
        tokens.add(expandSuffix(suffix.group(2)));
        continue;
      }

      if (i + 2 < rawTokens.length && rawTokens[i].matches("\\d+") && rawTokens[i + 1].matches("[A-Za-z]")
          && isStreetSuffix(rawTokens[i + 2]))
      {
        tokens.add(rawTokens[i] + rawTokens[++i].toUpperCase(Locale.ROOT));
        continue;
      }

      String token = expandDirection(rawTokens[i]);
      token = expandSuffix(token);
      if (token.equalsIgnoreCase("twp"))
        token = "Township";
      if (tokens.isEmpty() || !tokens.get(tokens.size() - 1).equalsIgnoreCase(token))
        tokens.add(token);
    }

    final int streetEnd = findStreetEnd(tokens);
    final int ordinalEnd = streetEnd < 0 ? tokens.size() - 1 : streetEnd;
    for (int i = 1; i < ordinalEnd; ++i)
    {
      if (isDirection(tokens.get(i - 1)) && tokens.get(i).matches("\\d+"))
        tokens.set(i, toOrdinal(tokens.get(i)));
    }
    return tokens;
  }

  private static int findStreetEnd(@NonNull List<String> tokens)
  {
    for (int i = 2; i < tokens.size(); ++i)
    {
      if (!isExpandedSuffix(tokens.get(i)))
        continue;
      if (i + 1 < tokens.size() && isDirection(tokens.get(i + 1)))
        return i + 1;
      return i;
    }
    return -1;
  }

  private static boolean isMatchingWrapper(char first, char last)
  {
    return first == '[' && last == ']' || first == '(' && last == ')' || first == '{' && last == '}';
  }

  @NonNull
  private static String expandDirection(@NonNull String token)
  {
    return switch (token.toLowerCase(Locale.ROOT))
    {
      case "n", "north" -> "North";
      case "s", "south" -> "South";
      case "e", "east" -> "East";
      case "w", "west" -> "West";
      case "ne", "northeast" -> "Northeast";
      case "nw", "northwest" -> "Northwest";
      case "se", "southeast" -> "Southeast";
      case "sw", "southwest" -> "Southwest";
      default -> token;
    };
  }

  @NonNull
  private static String expandSuffix(@NonNull String token)
  {
    return switch (token.toLowerCase(Locale.ROOT))
    {
      case "st", "street" -> "Street";
      case "ave", "av", "avenue" -> "Avenue";
      case "rd", "road" -> "Road";
      case "blvd", "boulevard" -> "Boulevard";
      case "dr", "drive" -> "Drive";
      case "ln", "lane" -> "Lane";
      case "ct", "court" -> "Court";
      case "cres", "cr", "cresent", "crescent" -> "Crescent";
      case "cir", "circle" -> "Circle";
      case "expy", "expressway" -> "Expressway";
      case "fwy", "freeway" -> "Freeway";
      case "hwy", "highway" -> "Highway";
      case "pkwy", "parkway" -> "Parkway";
      case "pl", "place" -> "Place";
      case "sq", "square" -> "Square";
      case "ter", "terr", "terrace" -> "Terrace";
      case "trl", "trail" -> "Trail";
      case "wy", "way" -> "Way";
      default -> token;
    };
  }

  private static boolean isStreetSuffix(@NonNull String token)
  {
    return isExpandedSuffix(expandSuffix(token));
  }

  private static boolean isExpandedSuffix(@NonNull String token)
  {
    return switch (token.toLowerCase(Locale.ROOT))
    {
      case "street", "avenue", "road", "boulevard", "drive", "lane", "court", "crescent", "circle", "expressway",
          "freeway", "highway", "parkway", "place", "square", "terrace", "trail", "way" ->
        true;
      default -> false;
    };
  }

  private static boolean isDirection(@NonNull String token)
  {
    return token.equalsIgnoreCase("North") || token.equalsIgnoreCase("South") || token.equalsIgnoreCase("East")
 || token.equalsIgnoreCase("West") || token.equalsIgnoreCase("Northeast") || token.equalsIgnoreCase("Northwest")
 || token.equalsIgnoreCase("Southeast") || token.equalsIgnoreCase("Southwest");
  }

  @NonNull
  private static String toOrdinal(@NonNull String token)
  {
    final Matcher ordinal = ORDINAL.matcher(token);
    if (ordinal.matches())
      return token;
    final String suffix;
    if (token.endsWith("11") || token.endsWith("12") || token.endsWith("13"))
      suffix = "th";
    else
    {
      suffix = switch (token.charAt(token.length() - 1))
      {
        case '1' -> "st";
        case '2' -> "nd";
        case '3' -> "rd";
        default -> "th";
      };
    }
    return token + suffix;
  }
}
