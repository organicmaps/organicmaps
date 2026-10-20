package app.organicmaps.search;

import androidx.annotation.NonNull;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

final class ContactResolutionPolicy
{
  private static final long RETRY_DELAY_MS = 5 * 60 * 1000;
  private final Map<String, Long> mMisses = new HashMap<>();
  // Full contact lookups are not limited to the viewport, unlike background marker searches.
  private final Map<String, Long> mForegroundMisses = new HashMap<>();
  private double mLeft;
  private double mBottom;
  private double mRight;
  private double mTop;
  private String mRegions;
  private long mMapRevision;

  boolean updateArea(@NonNull String regions, long mapRevision, double left, double bottom, double right, double top)
  {
    if (mapRevision != mMapRevision)
      mForegroundMisses.clear();
    double width = mRight - mLeft;
    double height = mTop - mBottom;
    double centerX = (left + right) / 2;
    double centerY = (bottom + top) / 2;
    if (regions.equals(mRegions) && mapRevision == mMapRevision && width > 0 && height > 0
        && Math.abs(centerX - (mLeft + mRight) / 2) <= width / 4
        && Math.abs(centerY - (mBottom + mTop) / 2) <= height / 4 && right - left <= width * 2
        && top - bottom <= height * 2)
      return false;
    mLeft = left;
    mBottom = bottom;
    mRight = right;
    mTop = top;
    mRegions = regions;
    mMapRevision = mapRevision;
    mMisses.clear();
    return true;
  }

  void recordMiss(@NonNull String key, long now)
  {
    mMisses.put(key, now);
  }

  boolean shouldRetry(@NonNull String key, long now)
  {
    return shouldRetry(mForegroundMisses, key, now) && shouldRetry(mMisses, key, now);
  }

  void recordForegroundMiss(@NonNull String key, long now)
  {
    mForegroundMisses.put(key, now);
  }

  boolean shouldRetryForeground(@NonNull String key, long now)
  {
    return shouldRetry(mForegroundMisses, key, now);
  }

  private static boolean shouldRetry(@NonNull Map<String, Long> misses, @NonNull String key, long now)
  {
    final Long missedAt = misses.get(key);
    if (missedAt == null)
      return true;
    if (now - missedAt < RETRY_DELAY_MS)
      return false;
    misses.remove(key);
    return true;
  }

  void forget(@NonNull String key)
  {
    mMisses.remove(key);
    mForegroundMisses.remove(key);
  }

  void clear()
  {
    mRegions = null;
    mMisses.clear();
    mForegroundMisses.clear();
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
