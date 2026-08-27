package app.organicmaps.intent.geo.navigation;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import app.organicmaps.sdk.util.log.Logger;
import java.util.ArrayList;
import java.util.List;

/// Defines things to avoid in navigation.
///
/// <b>Example:</b><p/>
/// `geo:0,0?q=googleplex&avoid=tf`<p/>
///
/// <b>Interpretation:</b><p/>
/// The user wants to navigate to Googleplex avoiding tolls and ferries.
public enum Avoid
{
  Ferries('f'),
  Highways('h'),
  Tolls('t');

  private static final String TAG = Avoid.class.getSimpleName();
  private final char mRaw;

  Avoid(char raw)
  {
    mRaw = raw;
  }

  @NonNull
  public static String getKey()
  {
    return "avoid";
  }

  @Nullable
  public static Avoid fromRaw(char raw)
  {
    for (Avoid avoid : values())
    {
      if (avoid.mRaw == raw)
        return avoid;
    }
    Logger.w(TAG, "Unknown avoid: " + raw);
    return null;
  }

  @Nullable
  public static List<Avoid> fromRaw(@Nullable String raw)
  {
    if (raw == null)
      return null;

    final List<Avoid> result = new ArrayList<>();
    for (char c : raw.toCharArray())
    {
      final Avoid avoid = fromRaw(c);
      if (avoid != null)
        result.add(avoid);
    }
    if (result.isEmpty())
      return null;
    return result;
  }
}
