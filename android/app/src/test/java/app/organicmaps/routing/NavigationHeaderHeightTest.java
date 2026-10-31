package app.organicmaps.routing;

import static org.junit.Assert.assertEquals;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

import android.view.View;
import android.view.ViewGroup;
import androidx.annotation.NonNull;
import org.junit.Test;

public class NavigationHeaderHeightTest
{
  @Test
  public void stackedLanesAndSpeedUseTheLowestBottom()
  {
    assertEquals(188, height(child(56, View.VISIBLE), child(130, View.VISIBLE), child(124, View.VISIBLE),
                             child(188, View.VISIBLE)));
  }

  @Test
  public void overlappingTurnAndSpeedAreNotAdded()
  {
    assertEquals(132, height(child(56, View.VISIBLE), child(132, View.VISIBLE), child(128, View.VISIBLE),
                             child(128, View.VISIBLE)));
  }

  @Test
  public void tallerTurnContainerDeterminesClearance()
  {
    assertEquals(200, height(child(56, View.VISIBLE), child(200, View.VISIBLE), child(124, View.VISIBLE),
                             child(188, View.VISIBLE)));
  }

  @Test
  public void hiddenChildrenDoNotContributeStaleBounds()
  {
    assertEquals(130, height(child(56, View.INVISIBLE), child(130, View.VISIBLE), child(200, View.GONE),
                             child(250, View.INVISIBLE)));
  }

  @Test
  public void noVisibleChildrenNeedNoClearance()
  {
    assertEquals(0, height(child(56, View.INVISIBLE), child(200, View.GONE)));
  }

  @Test
  public void emptyContentNeedsNoClearance()
  {
    assertEquals(0, height());
  }

  @Test
  public void childVisibilityChangesUpdateClearance()
  {
    View turn = child(130, View.VISIBLE);
    View speed = child(188, View.VISIBLE);
    assertEquals(188, height(turn, speed));
    when(speed.getVisibility()).thenReturn(View.GONE);
    assertEquals(130, height(turn, speed));
    when(speed.getVisibility()).thenReturn(View.VISIBLE);
    assertEquals(188, height(turn, speed));
  }

  @NonNull
  private static View child(int bottom, int visibility)
  {
    View child = mock(View.class);
    when(child.getBottom()).thenReturn(bottom);
    when(child.getVisibility()).thenReturn(visibility);
    return child;
  }

  private static int height(@NonNull View... children)
  {
    ViewGroup topFrame = mock(ViewGroup.class);
    when(topFrame.getChildCount()).thenReturn(children.length);
    for (int i = 0; i < children.length; ++i)
      when(topFrame.getChildAt(i)).thenReturn(children[i]);
    return NavigationController.lowestVisibleChildBottom(topFrame);
  }
}
