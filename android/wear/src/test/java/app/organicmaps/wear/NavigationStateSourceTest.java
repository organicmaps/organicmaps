package app.organicmaps.wear;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

import app.organicmaps.wear.protocol.WearNavigationData;
import app.organicmaps.wear.protocol.WearNavigationMode;
import app.organicmaps.wear.protocol.WearNavigationState;
import com.google.android.gms.wearable.DataEvent;
import com.google.android.gms.wearable.DataMap;
import com.google.android.gms.wearable.Node;
import java.util.List;
import org.junit.Test;

public class NavigationStateSourceTest
{
  @Test
  public void emptyCollectionHasNoCompanion()
  {
    assertNull(NavigationStateSource.selectCompanion(List.of()));
  }

  @Test
  public void nearbyNodeWins()
  {
    final Node remote = node("a", false);
    final Node nearby = node("z", true);

    assertEquals(nearby, NavigationStateSource.selectCompanion(List.of(remote, nearby)));
  }

  @Test
  public void nodeIdMakesSelectionDeterministic()
  {
    final Node first = node("a", true);
    final Node second = node("b", true);

    assertEquals(first, NavigationStateSource.selectCompanion(List.of(second, first)));
    assertEquals(first, NavigationStateSource.selectCompanion(List.of(first, second)));
  }

  @Test
  public void changedOrDeletedItemRequiresRefresh()
  {
    assertTrue(NavigationStateSource.requiresRefresh(List.of(event(DataEvent.TYPE_CHANGED))));
    assertTrue(NavigationStateSource.requiresRefresh(List.of(event(DataEvent.TYPE_DELETED))));
  }

  @Test
  public void emptyOrUnrelatedEventDoesNotRequireRefresh()
  {
    assertFalse(NavigationStateSource.requiresRefresh(List.of()));
    assertFalse(NavigationStateSource.requiresRefresh(List.of(event(0))));
  }

  @Test
  public void navigationDataMapDecodes()
  {
    final WearNavigationState state =
        NavigationStateSource.decodeNavigationState(dataMap(WearNavigationData.VERSION, "NAVIGATION"));

    assertEquals(WearNavigationMode.NAVIGATION, state.getMode());
  }

  @Test
  public void normalDataMapDecodes()
  {
    final WearNavigationState state =
        NavigationStateSource.decodeNavigationState(dataMap(WearNavigationData.VERSION, "NORMAL"));

    assertEquals(WearNavigationMode.NORMAL, state.getMode());
  }

  @Test
  public void versionMismatchReturnsNull()
  {
    assertNull(NavigationStateSource.decodeNavigationState(dataMap(999, "NAVIGATION")));
  }

  @Test
  public void missingModeReturnsNull()
  {
    final DataMap dataMap = new DataMap();
    dataMap.putInt(WearNavigationData.KEY_VERSION, WearNavigationData.VERSION);

    assertNull(NavigationStateSource.decodeNavigationState(dataMap));
  }

  @Test
  public void unknownModeReturnsNull()
  {
    assertNull(
        NavigationStateSource.decodeNavigationState(
            dataMap(WearNavigationData.VERSION, "BOGUS")));
  }

  @Test
  public void unknownFieldIsIgnored()
  {
    final DataMap dataMap = dataMap(WearNavigationData.VERSION, "NAVIGATION");
    dataMap.putBoolean("unknown", true);

    assertEquals(
        WearNavigationMode.NAVIGATION,
        NavigationStateSource.decodeNavigationState(dataMap).getMode());
  }

  private static DataMap dataMap(int version, String mode)
  {
    final DataMap dataMap = new DataMap();
    dataMap.putInt(WearNavigationData.KEY_VERSION, version);
    dataMap.putString(WearNavigationData.KEY_MODE, mode);
    return dataMap;
  }

  private static Node node(String id, boolean nearby)
  {
    final Node node = mock(Node.class);
    when(node.getId()).thenReturn(id);
    when(node.isNearby()).thenReturn(nearby);
    return node;
  }

  private static DataEvent event(int type)
  {
    final DataEvent event = mock(DataEvent.class);
    when(event.getType()).thenReturn(type);
    return event;
  }
}
