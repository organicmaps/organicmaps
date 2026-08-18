package app.organicmaps.wear;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import app.organicmaps.wear.protocol.WearNavigationData;
import app.organicmaps.wear.protocol.WearNavigationMode;
import app.organicmaps.wear.protocol.WearNavigationState;
import com.google.android.gms.wearable.DataEvent;
import com.google.android.gms.wearable.DataMap;
import com.google.android.gms.wearable.Node;
import java.util.Collection;

final class NavigationStateSource
{
  @Nullable
  static Node selectCompanion(@NonNull Collection<Node> nodes)
  {
    Node selected = null;
    for (final Node node : nodes)
    {
      if (selected == null || compare(node, selected) < 0)
        selected = node;
    }
    return selected;
  }

  static boolean requiresRefresh(@NonNull Iterable<DataEvent> events)
  {
    for (final DataEvent event : events)
    {
      if (event.getType() == DataEvent.TYPE_CHANGED || event.getType() == DataEvent.TYPE_DELETED)
        return true;
    }
    return false;
  }

  @Nullable
  static WearNavigationState decodeNavigationState(@NonNull DataMap dataMap)
  {
    if (dataMap.getInt(WearNavigationData.KEY_VERSION, -1) != WearNavigationData.VERSION)
      return null;

    final String modeName = dataMap.getString(WearNavigationData.KEY_MODE);
    if (modeName == null)
      return null;

    try
    {
      final WearNavigationMode mode = WearNavigationMode.valueOf(modeName);
      if (mode == WearNavigationMode.NORMAL)
        return WearNavigationState.normal();
      if (mode == WearNavigationMode.NAVIGATION)
        return WearNavigationState.navigation();
      return null;
    }
    catch (IllegalArgumentException e)
    {
      return null;
    }
  }

  private static int compare(@NonNull Node lhs, @NonNull Node rhs)
  {
    if (lhs.isNearby() != rhs.isNearby())
      return lhs.isNearby() ? -1 : 1;
    return lhs.getId().compareTo(rhs.getId());
  }

  private NavigationStateSource() {}
}
