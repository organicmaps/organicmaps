package app.organicmaps.search;

import androidx.annotation.NonNull;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

final class ContactAddressQueryNormalizer
{
  private static final Pattern UNIT_PREFIX =
      Pattern.compile("(?i)^\\s*(?:(?:unit|suite|apt|apartment)\\s+|#)\\w+[,.\\s]+(?=\\d+\\s)");
  private static final Pattern HYPHENATED_UNIT = Pattern.compile("^\\s*\\w+-(?=\\d+\\s)");
  private static final Pattern SLASHED_UNIT = Pattern.compile("^\\s*\\w+/(?=\\d+\\s)");
  private static final Pattern TRAILING_UNIT =
      Pattern.compile("(?i)\\s+(?:(?:unit|suite|apt|apartment)\\s*|#)\\w+\\s*$");
  private static final Pattern ADDRESS_UNIT =
      Pattern.compile("(?i)(?:^|\\s)(?:(?:unit|suite|apt|apartment)\\s*|#)\\w+(?=\\s|$)");
  private static final Pattern CANADIAN_POSTAL_CODE =
      Pattern.compile("(?i)\\b[ABCEGHJ-NPRSTVXY]\\d[ABCEGHJ-NPRSTV-Z]\\s*\\d[ABCEGHJ-NPRSTV-Z]\\d\\b");
  private static final Pattern US_ZIP_CODE =
      Pattern.compile("(?i)\\s+\\d{5}(?:-\\d{4})?(?=\\s*(?:USA|U\\.?S\\.?A\\.?|United States(?: of America)?)?\\s*$)");
  private static final Pattern TRAILING_COUNTRY =
      Pattern.compile("(?i)\\s+(?:Canada|USA|U\\.?S\\.?A\\.?|United States(?: of America)?)\\s*$");
  private static final Pattern TRAILING_REGION_CODE = Pattern.compile(
      "(?i)\\s+(?:AB|BC|MB|NB|NL|NS|NT|NU|ON|PE|QC|SK|YT|" +
      "AL|AK|AZ|AR|CA|CO|CT|DE|DC|FL|GA|HI|ID|IL|IN|IA|KS|KY|LA|ME|MD|MA|MI|MN|MS|MO|MT|NE|NV|NH|NJ|" +
      "NM|NY|NC|ND|OH|OK|OR|PA|RI|SC|SD|TN|TX|UT|VT|VA|WA|WV|WI|WY)\\s*$");
  private static final Pattern ORDINAL = Pattern.compile("\\d+(?:st|nd|rd|th)", Pattern.CASE_INSENSITIVE);

  private ContactAddressQueryNormalizer() {}

  @NonNull
  static String normalizeStreet(@NonNull String value)
  {
    String street = value.trim();
    street = UNIT_PREFIX.matcher(street).replaceFirst("");
    street = HYPHENATED_UNIT.matcher(street).replaceFirst("");
    street = firstAddressLine(street).trim();
    street = TRAILING_UNIT.matcher(street).replaceFirst("");
    street = street.replaceAll("[,.]", " ").replaceAll("\\s+", " ").trim();
    if (street.isEmpty())
      return street;

    final String[] rawTokens = street.split(" ");
    final List<String> tokens = new ArrayList<>(rawTokens.length);
    for (String rawToken : rawTokens)
      tokens.add(expandDirection(rawToken));

    final int suffixIndex = tokens.size() - 1;
    tokens.set(suffixIndex, expandSuffix(tokens.get(suffixIndex)));
    for (int i = 1; i < suffixIndex; ++i)
    {
      if (isDirection(tokens.get(i - 1)) && tokens.get(i).matches("\\d+"))
        tokens.set(i, toOrdinal(tokens.get(i)));
    }
    return String.join(" ", tokens);
  }

  static boolean looksLikeAddressQuery(@NonNull String value)
  {
    return normalizeAddressQuery(value).matches("\\d+\\s+\\S.*");
  }

  @NonNull
  static String normalizeAddressQuery(@NonNull String value)
  {
    String query = value.trim();
    while (query.length() >= 2 && isMatchingWrapper(query.charAt(0), query.charAt(query.length() - 1)))
      query = query.substring(1, query.length() - 1).trim();

    query = HYPHENATED_UNIT.matcher(query).replaceFirst("");
    query = SLASHED_UNIT.matcher(query).replaceFirst("");
    query = query.replaceAll("[\\[\\]{}()]", " ").replaceAll("[.,;:]", " ");
    query = ADDRESS_UNIT.matcher(query).replaceAll(" ");
    query = CANADIAN_POSTAL_CODE.matcher(query).replaceAll(" ");
    query = US_ZIP_CODE.matcher(query).replaceAll(" ");
    query = TRAILING_COUNTRY.matcher(query).replaceFirst("");
    query = TRAILING_REGION_CODE.matcher(query).replaceFirst("");
    query = query.replaceAll("\\s+", " ").trim();
    if (!query.matches("\\d+\\s+\\S.*"))
      return value.trim();

    final String[] rawTokens = query.split(" ");
    final List<String> tokens = new ArrayList<>(rawTokens.length);
    int streetSuffixIndex = -1;
    for (int i = 0; i < rawTokens.length; ++i)
    {
      final String expandedSuffix = expandSuffix(rawTokens[i]);
      if (streetSuffixIndex < 0 && i >= 2 &&
          (!expandedSuffix.equals(rawTokens[i]) || isExpandedSuffix(rawTokens[i])))
      {
        streetSuffixIndex = i;
        tokens.add(expandedSuffix);
      }
      else
        tokens.add(rawTokens[i]);
    }

    int streetEnd = streetSuffixIndex < 0 ? tokens.size() - 1 : streetSuffixIndex;
    if (streetSuffixIndex >= 0 && streetEnd + 1 < tokens.size() &&
        isDirection(expandDirection(tokens.get(streetEnd + 1))))
      ++streetEnd;
    for (int i = 1; i <= streetEnd; ++i)
      tokens.set(i, expandDirection(tokens.get(i)));
    for (int i = 1; i < streetEnd; ++i)
    {
      if (isDirection(tokens.get(i - 1)) && tokens.get(i).matches("\\d+"))
        tokens.set(i, toOrdinal(tokens.get(i)));
    }
    return String.join(" ", tokens);
  }

  private static boolean isMatchingWrapper(char first, char last)
  {
    return first == '[' && last == ']' || first == '(' && last == ')' || first == '{' && last == '}';
  }

  @NonNull
  private static String firstAddressLine(@NonNull String value)
  {
    int end = value.length();
    for (char separator : new char[] {'\n', '\r', ','})
    {
      final int index = value.indexOf(separator);
      if (index >= 0)
        end = Math.min(end, index);
    }
    return value.substring(0, end);
  }

  @NonNull
  private static String expandDirection(@NonNull String token)
  {
    return switch (token.toLowerCase(Locale.ROOT))
    {
      case "n" -> "North";
      case "s" -> "South";
      case "e" -> "East";
      case "w" -> "West";
      case "ne" -> "Northeast";
      case "nw" -> "Northwest";
      case "se" -> "Southeast";
      case "sw" -> "Southwest";
      default -> token;
    };
  }

  @NonNull
  private static String expandSuffix(@NonNull String token)
  {
    return switch (token.toLowerCase(Locale.ROOT))
    {
      case "st" -> "Street";
      case "ave", "av" -> "Avenue";
      case "rd" -> "Road";
      case "blvd" -> "Boulevard";
      case "dr" -> "Drive";
      case "ln" -> "Lane";
      case "ct" -> "Court";
      case "cres" -> "Crescent";
      case "cir" -> "Circle";
      case "expy" -> "Expressway";
      case "fwy" -> "Freeway";
      case "hwy" -> "Highway";
      case "pkwy" -> "Parkway";
      case "pl" -> "Place";
      case "sq" -> "Square";
      case "ter", "terr" -> "Terrace";
      case "trl" -> "Trail";
      case "wy" -> "Way";
      default -> token;
    };
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
