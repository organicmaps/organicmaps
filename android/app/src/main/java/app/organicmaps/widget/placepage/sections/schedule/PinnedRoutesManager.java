package app.organicmaps.widget.placepage.sections.schedule;

import android.content.Context;
import android.content.SharedPreferences;
import androidx.annotation.NonNull;
import java.util.HashSet;
import java.util.Set;

/**
 * Manages pinned (favorite) transport routes.
 * Pinned routes appear at the top of the schedule list.
 */
public class PinnedRoutesManager
{
  private static final String PREFS_NAME = "schedule_prefs";
  private static final String PREFS_KEY_PINNED = "pinned_route_ids";

  @NonNull
  private final SharedPreferences mPrefs;

  public PinnedRoutesManager(@NonNull Context context)
  {
    mPrefs = context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE);
  }

  @NonNull
  public Set<String> getPinnedRouteIds()
  {
    return new HashSet<>(mPrefs.getStringSet(PREFS_KEY_PINNED, new HashSet<>()));
  }

  public boolean isPinned(@NonNull String routeId)
  {
    return getPinnedRouteIds().contains(routeId);
  }

  public void togglePin(@NonNull String routeId)
  {
    Set<String> pinned = getPinnedRouteIds();
    if (pinned.contains(routeId))
      pinned.remove(routeId);
    else
      pinned.add(routeId);

    mPrefs.edit().putStringSet(PREFS_KEY_PINNED, pinned).apply();
  }

  public void pin(@NonNull String routeId)
  {
    Set<String> pinned = getPinnedRouteIds();
    pinned.add(routeId);
    mPrefs.edit().putStringSet(PREFS_KEY_PINNED, pinned).apply();
  }

  public void unpin(@NonNull String routeId)
  {
    Set<String> pinned = getPinnedRouteIds();
    pinned.remove(routeId);
    mPrefs.edit().putStringSet(PREFS_KEY_PINNED, pinned).apply();
  }
}
