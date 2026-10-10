package app.organicmaps.routing;

import static org.mockito.Mockito.CALLS_REAL_METHODS;
import static org.mockito.Mockito.clearInvocations;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.view.View;
import android.widget.RadioGroup;
import android.widget.TextView;
import app.organicmaps.sdk.Router;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.routing.RoutingOptions;
import app.organicmaps.sdk.settings.RoadType;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.EnumSet;
import org.junit.Test;
import org.mockito.ArgumentCaptor;
import org.mockito.MockedStatic;

public class RoutingPlanOptionsTest
{
  @Test
  public void modeUpdatesRefreshTheBadgeAndKeepTransitSettings() throws ReflectiveOperationException
  {
    RoutingPlanFragment fragment = mock(RoutingPlanFragment.class, CALLS_REAL_METHODS);
    when(fragment.getView()).thenReturn(mock(View.class));
    TextView badge = mock(TextView.class);
    View container = mock(View.class);
    View settingsButton = mock(View.class);
    setField(fragment, "mDrivingOptionsBadge", badge);
    setField(fragment, "mDrivingOptionsContainer", container);
    setField(fragment, "mDrivingOptionsBtn", settingsButton);
    setField(fragment, "mRoutingContentActive", true);
    setField(fragment, "mRouterTypes", mock(RadioGroup.class));
    setField(fragment, "mRoutingBottomMenuController", mock(RoutingBottomMenuController.class));
    RoutingController controller = mock(RoutingController.class);

    try (MockedStatic<RoutingController> routing = mockStatic(RoutingController.class);
         MockedStatic<RoutingOptions> options = mockStatic(RoutingOptions.class, CALLS_REAL_METHODS))
    {
      routing.when(RoutingController::get).thenReturn(controller);
      options.when(() -> RoutingOptions.getActiveRoadTypes(Router.Vehicle))
          .thenReturn(EnumSet.of(RoadType.Toll, RoadType.Motorway, RoadType.Dirty));
      options.when(() -> RoutingOptions.getActiveRoadTypes(Router.Bicycle)).thenReturn(EnumSet.noneOf(RoadType.class));
      options.when(() -> RoutingOptions.getActiveRoadTypes(Router.Pedestrian)).thenReturn(EnumSet.of(RoadType.Ferry));
      options.when(() -> RoutingOptions.getActiveRoadTypes(Router.Transit)).thenReturn(EnumSet.noneOf(RoadType.class));
      options.when(() -> RoutingOptions.getActiveRoadTypes(Router.Ruler)).thenReturn(EnumSet.noneOf(RoadType.class));

      updateProgress(fragment, Router.Vehicle);
      verify(badge).setText("3");
      verify(container).setAlpha(1.0f);
      verify(settingsButton).setEnabled(true);
      clearInvocations(badge, container, settingsButton);

      updateProgress(fragment, Router.Bicycle);
      verify(badge).setVisibility(View.GONE);
      verify(container).setAlpha(1.0f);
      verify(settingsButton).setEnabled(true);
      clearInvocations(badge, container, settingsButton);

      updateProgress(fragment, Router.Pedestrian);
      verify(badge).setText("1");
      verify(container).setAlpha(1.0f);
      verify(settingsButton).setEnabled(true);
      clearInvocations(badge, container, settingsButton);

      updateProgress(fragment, Router.Transit);
      verify(badge).setVisibility(View.GONE);
      verify(container).setAlpha(1.0f);
      verify(settingsButton).setEnabled(true);
      clearInvocations(badge, container, settingsButton);

      updateProgress(fragment, Router.Ruler);
      verify(badge).setVisibility(View.GONE);
      verify(container).setAlpha(0.5f);
      verify(settingsButton).setEnabled(false);
    }
  }

  @Test
  public void aPartialPlanUpdatesItsBadgeWithoutABuildCallback() throws ReflectiveOperationException
  {
    RoutingPlanFragment fragment = mock(RoutingPlanFragment.class, CALLS_REAL_METHODS);
    TextView badge = mock(TextView.class);
    View container = mock(View.class);
    View settingsButton = mock(View.class);
    RadioGroup types = mock(RadioGroup.class);
    View button = mock(View.class);
    when(types.findViewById(1)).thenReturn(button);
    setField(fragment, "mDrivingOptionsBadge", badge);
    setField(fragment, "mDrivingOptionsContainer", container);
    setField(fragment, "mDrivingOptionsBtn", settingsButton);
    setField(fragment, "mRoutingContentActive", true);
    setField(fragment, "mRouterTypes", types);
    RoutingController controller = mock(RoutingController.class);
    try (MockedStatic<RoutingController> routing = mockStatic(RoutingController.class);
         MockedStatic<RoutingOptions> options = mockStatic(RoutingOptions.class, CALLS_REAL_METHODS))
    {
      routing.when(RoutingController::get).thenReturn(controller);
      options.when(() -> RoutingOptions.getActiveRoadTypes(Router.Bicycle)).thenReturn(EnumSet.noneOf(RoadType.class));
      Method install = RoutingPlanFragment.class.getDeclaredMethod("setRouterClick", int.class, Router.class);
      install.setAccessible(true);
      install.invoke(fragment, 1, Router.Bicycle);
      ArgumentCaptor<View.OnClickListener> listener = ArgumentCaptor.forClass(View.OnClickListener.class);
      verify(button).setOnClickListener(listener.capture());
      listener.getValue().onClick(button);
      verify(controller).setRouterType(Router.Bicycle);
      verify(badge).setVisibility(View.GONE);
      verify(container).setAlpha(1.0f);
      verify(settingsButton).setEnabled(true);
    }
  }

  private static void updateProgress(RoutingPlanFragment fragment, Router router) throws ReflectiveOperationException
  {
    Method method = RoutingPlanFragment.class.getDeclaredMethod("updateBuildProgress", int.class, Router.class);
    method.setAccessible(true);
    method.invoke(fragment, 0, router);
  }

  private static void setField(RoutingPlanFragment fragment, String name, Object value)
      throws ReflectiveOperationException
  {
    Field field = RoutingPlanFragment.class.getDeclaredField(name);
    field.setAccessible(true);
    field.set(fragment, value);
  }
}
