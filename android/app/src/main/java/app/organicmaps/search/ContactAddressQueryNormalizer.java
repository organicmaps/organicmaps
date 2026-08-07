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
  private static final Pattern TRAILING_UNIT =
      Pattern.compile("(?i)\\s+(?:(?:unit|suite|apt|apartment)\\s*|#)\\w+\\s*$");
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
      case "hwy" -> "Highway";
      case "pkwy" -> "Parkway";
      case "pl" -> "Place";
      case "ter", "terr" -> "Terrace";
      case "trl" -> "Trail";
      default -> token;
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
