package app.organicmaps.routing;

import static org.junit.Assert.assertEquals;
import static org.mockito.Mockito.CALLS_REAL_METHODS;
import static org.mockito.Mockito.anyInt;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.doNothing;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.view.View;
import android.view.ViewGroup;
import androidx.annotation.NonNull;
import app.organicmaps.maplayer.MapButtonsViewModel;
import app.organicmaps.sdk.routing.RoutingController;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.concurrent.atomic.AtomicInteger;
import org.junit.Test;
import org.mockito.MockedStatic;

public class NavigationHeaderHeightTest
{
  @Test
  public void stackedLanesAndSpeedUseTheLowestBottom() throws ReflectiveOperationException
  {
    assertEquals(188, height(View.VISIBLE, child(56, View.VISIBLE), child(130, View.VISIBLE), child(124, View.VISIBLE),
                             child(188, View.VISIBLE)));
  }

  @Test
  public void overlappingTurnAndSpeedAreNotAdded() throws ReflectiveOperationException
  {
    assertEquals(132, height(View.VISIBLE, child(56, View.VISIBLE), child(132, View.VISIBLE), child(128, View.VISIBLE),
                             child(128, View.VISIBLE)));
  }

  @Test
  public void tallerTurnContainerDeterminesClearance() throws ReflectiveOperationException
  {
    assertEquals(200, height(View.VISIBLE, child(56, View.VISIBLE), child(200, View.VISIBLE), child(124, View.VISIBLE),
                             child(188, View.VISIBLE)));
  }

  @Test
  public void hiddenChildrenDoNotContributeStaleBounds() throws ReflectiveOperationException
  {
    assertEquals(130, height(View.VISIBLE, child(56, View.INVISIBLE), child(130, View.VISIBLE), child(200, View.GONE),
                             child(250, View.INVISIBLE)));
  }

  @Test
  public void noVisibleChildrenNeedNoClearance() throws ReflectiveOperationException
  {
    assertEquals(0, height(View.VISIBLE, child(56, View.INVISIBLE), child(200, View.GONE)));
  }

  @Test
  public void hiddenNavigationNeedsNoClearance() throws ReflectiveOperationException
  {
    assertEquals(0, height(View.GONE, child(200, View.VISIBLE)));
    assertEquals(0, height(View.INVISIBLE, child(200, View.VISIBLE)));
  }

  @Test
  public void showingInvisibleNavigationPublishesExistingBounds() throws ReflectiveOperationException
  {
    checkVisibilityTransitions(View.INVISIBLE);
  }

  @Test
  public void showingGoneNavigationPublishesExistingBounds() throws ReflectiveOperationException
  {
    checkVisibilityTransitions(View.GONE);
  }

  private static void checkVisibilityTransitions(int initialVisibility) throws ReflectiveOperationException
  {
    NavigationController controller = mock(NavigationController.class, CALLS_REAL_METHODS);
    View frame = mock(View.class);
    AtomicInteger visibility = new AtomicInteger(initialVisibility);
    when(frame.getVisibility()).thenAnswer(invocation -> visibility.get());
    doAnswer(invocation -> {
      visibility.set(invocation.getArgument(0));
      return null;
    })
        .when(frame)
        .setVisibility(anyInt());
    ViewGroup topFrame = mock(ViewGroup.class);
    View content = child(188, View.VISIBLE);
    when(topFrame.getChildCount()).thenReturn(1);
    when(topFrame.getChildAt(0)).thenReturn(content);
    MapButtonsViewModel viewModel = mock(MapButtonsViewModel.class);
    setField(controller, "mFrame", frame);
    setField(controller, "mTopFrame", topFrame);
    setField(controller, "mMapButtonsViewModel", viewModel);
    doNothing().when(controller).collapseNavMenu();
    doNothing().when(controller).update(null);

    try (MockedStatic<RoutingController> routing = mockStatic(RoutingController.class))
    {
      routing.when(RoutingController::get).thenReturn(mock(RoutingController.class));
      controller.show(true);
      verify(viewModel).setTopHeaderHeight(188);
      controller.show(false);
      verify(viewModel).setTopHeaderHeight(0);
      controller.show(true);
      verify(viewModel, times(2)).setTopHeaderHeight(188);
    }
  }

  @NonNull
  private static View child(int bottom, int visibility)
  {
    View child = mock(View.class);
    when(child.getBottom()).thenReturn(bottom);
    when(child.getVisibility()).thenReturn(visibility);
    return child;
  }

  private static int height(int visibility, @NonNull View... children) throws ReflectiveOperationException
  {
    NavigationController controller = mock(NavigationController.class, CALLS_REAL_METHODS);
    View frame = mock(View.class);
    when(frame.getVisibility()).thenReturn(visibility);
    ViewGroup topFrame = mock(ViewGroup.class);
    when(topFrame.getChildCount()).thenReturn(children.length);
    for (int i = 0; i < children.length; ++i)
      when(topFrame.getChildAt(i)).thenReturn(children[i]);
    setField(controller, "mFrame", frame);
    setField(controller, "mTopFrame", topFrame);
    Method method = NavigationController.class.getDeclaredMethod("computeNavContentHeight");
    method.setAccessible(true);
    return (int) method.invoke(controller);
  }

  private static void setField(NavigationController controller, String name, Object value)
      throws ReflectiveOperationException
  {
    Field field = NavigationController.class.getDeclaredField(name);
    field.setAccessible(true);
    field.set(controller, value);
  }
}
