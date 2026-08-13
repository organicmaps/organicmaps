package app.organicmaps.search;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import java.text.Normalizer;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

final class ContactAddressNormalizer
{
  private static final Pattern UNIT_PREFIX =
      Pattern.compile("(?i)^\\s*(?:(?:unit|suite|apt\\.?|apartment|cpo)\\s*#?\\s*|#)\\w+[,.\\s]+(?=\\d+\\s)");
  private static final Pattern BASEMENT_PREFIX = Pattern.compile("(?i)^\\s*basement\\s+(?=\\d+\\s)");
  private static final Pattern LEADING_NUMBER_SEPARATOR =
      Pattern.compile("^\\s*(\\d+[A-Za-z]?)\\s*[-/]\\s*(\\d+[A-Za-z]?)\\s+(.+)$");
  private static final Pattern TRAILING_UNIT =
      Pattern.compile("(?i)\\s+(?:(?:unit|suite|apt\\.?|apartment)\\s*|#)\\w+\\s*$");
  private static final Pattern ADDRESS_UNIT =
      Pattern.compile("(?i)(?:^|\\s)(?:(?:unit|suite|apt\\.?|apartment|cpo)\\s*#?\\s*|#)\\w+(?=\\s|$)");
  private static final Pattern ADDRESS_BASEMENT = Pattern.compile("(?i)(?:^|\\s)basement(?=\\s|$)");
  private static final Pattern CANADIAN_POSTAL_CODE =
      Pattern.compile("(?i)\\b[ABCEGHJ-NPRSTVXY]\\d[ABCEGHJ-NPRSTV-Z]\\s*\\d[ABCEGHJ-NPRSTV-Z]\\d\\b");
  private static final Pattern US_ZIP_CODE =
      Pattern.compile("(?i)\\s+\\d{5}(?:-\\d{4})?(?=\\s*(?:USA|U\\.?S\\.?A\\.?|United States(?: of America)?)?\\s*$)");
  private static final Pattern TRAILING_COUNTRY =
      Pattern.compile("(?i)(?:\\s+(?:Canada|USA|U\\.?S\\.?A\\.?|United States(?: of America)?))+\\s*$");
  private static final Pattern TRAILING_REGION_CODE = Pattern.compile(
      "(?i)\\s+(?:AB|BC|MB|NB|NL|NS|NT|NU|ON|PE|QC|SK|YT|" +
      "AL|AK|AZ|AR|CA|CO|CT|DE|DC|FL|GA|HI|ID|IL|IN|IA|KS|KY|LA|ME|MD|MA|MI|MN|MS|MO|MT|NE|NV|NH|NJ|" +
      "NM|NY|NC|ND|OH|OK|OR|PA|RI|SC|SD|TN|TX|UT|VT|VA|WA|WV|WI|WY)\\s*$");
  private static final Pattern ORDINAL = Pattern.compile("(\\d+)(?:st|nd|rd|th)", Pattern.CASE_INSENSITIVE);
  private static final Pattern ATTACHED_SUFFIX = Pattern.compile(
      "(?i)^(\\d+[A-Za-z]?)(st|ave|av|rd|blvd|dr|ln|ct|cres|cr|cir|pl|ter|terr|trl|wy)$");
  private static final Pattern ATTACHED_DIRECTIONAL_NUMBER =
      Pattern.compile("(?i)^([NSEW])(\\d+)(?:st|nd|rd|th)?$");
  private static final Pattern ADDRESS_START = Pattern.compile("(?i)\\b\\d+[A-Za-z]?\\s+(?=\\S)");
  private static final Pattern BARE_UNIT_STREET = Pattern.compile("^(\\d+)\\s+(\\d+[A-Za-z]?)\\s+(.+)$");

  private ContactAddressNormalizer() {}

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
  static String normalizeAddressQuery(@NonNull String value)
  {
    String query = value.trim();
    while (query.length() >= 2 && isMatchingWrapper(query.charAt(0), query.charAt(query.length() - 1)))
      query = query.substring(1, query.length() - 1).trim();

    query = prepareAddress(query);
    query = query.replaceAll("(?i)\\bB\\.\\s*C\\.?\\b", "BC");
    query = query.replaceAll("[\\[\\]{}()]", " ").replaceAll("[.,;:|!]", " ");
    query = ADDRESS_UNIT.matcher(query).replaceAll(" ");
    query = ADDRESS_BASEMENT.matcher(query).replaceAll(" ");
    query = CANADIAN_POSTAL_CODE.matcher(query).replaceAll(" ");
    query = US_ZIP_CODE.matcher(query).replaceAll(" ");
    query = TRAILING_COUNTRY.matcher(query).replaceFirst("");
    query = TRAILING_REGION_CODE.matcher(query).replaceFirst("");
    query = query.replaceAll("\\s+", " ").trim();
    if (!query.matches("\\d+[\\p{L}]?\\s+\\S.*"))
      return value.trim();
    return String.join(" ", normalizeTokens(query));
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
    final String cleaned = value.replaceAll("[\\[\\]{}()]", " ").replaceAll("[.,;:|!]", " ")
                                .replaceAll("\\s+", " ").trim();
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

      if (i + 2 < rawTokens.length && rawTokens[i].matches("\\d+") && rawTokens[i + 1].matches("[A-Za-z]") &&
          isStreetSuffix(rawTokens[i + 2]))
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
      case "street", "avenue", "road", "boulevard", "drive", "lane", "court", "crescent", "circle",
           "expressway", "freeway", "highway", "parkway", "place", "square", "terrace", "trail", "way" -> true;
      default -> false;
    };
  }

  private static boolean isDirection(@NonNull String token)
  {
    return token.equalsIgnoreCase("North") || token.equalsIgnoreCase("South") || token.equalsIgnoreCase("East") ||
           token.equalsIgnoreCase("West") || token.equalsIgnoreCase("Northeast") ||
           token.equalsIgnoreCase("Northwest") || token.equalsIgnoreCase("Southeast") ||
           token.equalsIgnoreCase("Southwest");
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
