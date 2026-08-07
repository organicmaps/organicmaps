package app.organicmaps.search;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import java.util.ArrayList;
import java.util.List;

final class ContactAddressFormatter
{
  private ContactAddressFormatter() {}

  @NonNull
  static String format(@Nullable String formattedAddress, @Nullable String... addressParts)
  {
    if (formattedAddress != null && !formattedAddress.isEmpty())
      return joinNonEmpty(formattedAddress.split("[\\r\\n]+"));
    return joinNonEmpty(addressParts);
  }

  @NonNull
  private static String joinNonEmpty(@Nullable String... parts)
  {
    final List<String> normalized = new ArrayList<>();
    if (parts != null)
    {
      for (String part : parts)
      {
        if (part == null)
          continue;
        final String value = part.trim();
        if (!value.isEmpty() && !normalized.contains(value))
          normalized.add(value);
      }
    }
    return String.join(", ", normalized);
  }
}
