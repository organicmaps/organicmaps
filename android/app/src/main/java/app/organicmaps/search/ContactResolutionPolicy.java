package app.organicmaps.search;

import androidx.annotation.NonNull;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

final class ContactResolutionPolicy
{
  private static final long RETRY_DELAY_MS = 5 * 60 * 1000;
  private final Map<String, Long> mMisses = new HashMap<>();
  private double mLeft;
  private double mBottom;
  private double mRight;
  private double mTop;
  private String mRegions;
  private long mMapVersion;

  boolean updateArea(@NonNull String regions, long mapVersion, double left, double bottom, double right, double top)
  {
    double width = mRight - mLeft;
    double height = mTop - mBottom;
    double centerX = (left + right) / 2;
    double centerY = (bottom + top) / 2;
    if (regions.equals(mRegions) && mapVersion == mMapVersion && width > 0 && height > 0
        && Math.abs(centerX - (mLeft + mRight) / 2) <= width / 4
        && Math.abs(centerY - (mBottom + mTop) / 2) <= height / 4 && right - left <= width * 2
        && top - bottom <= height * 2)
      return false;
    mLeft = left;
    mBottom = bottom;
    mRight = right;
    mTop = top;
    mRegions = regions;
    mMapVersion = mapVersion;
    mMisses.clear();
    return true;
  }

  void recordMiss(@NonNull String key, long now)
  {
    mMisses.put(key, now);
  }

  boolean shouldRetry(@NonNull String key, long now)
  {
    Long missedAt = mMisses.get(key);
    return missedAt == null || now - missedAt >= RETRY_DELAY_MS;
  }

  void forget(@NonNull String key)
  {
    mMisses.remove(key);
  }

  void clear()
  {
    mRegions = null;
    mMisses.clear();
  }

  static int regionScore(@NonNull List<String> context, @NonNull List<String> region)
  {
    // A locality matching the map name is only a priority hint, never an exclusion.
    int score = 0;
    for (String token : context)
      if (region.contains(token))
        ++score;
    return score;
  }
}
