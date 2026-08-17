package app.organicmaps.wear;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.google.android.gms.wearable.DataEvent;
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

  private static int compare(@NonNull Node lhs, @NonNull Node rhs)
  {
    if (lhs.isNearby() != rhs.isNearby())
      return lhs.isNearby() ? -1 : 1;
    return lhs.getId().compareTo(rhs.getId());
  }

  private NavigationStateSource() {}
}
