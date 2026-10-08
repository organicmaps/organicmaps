package app.organicmaps.routing;

import static org.mockito.Mockito.CALLS_REAL_METHODS;
import static org.mockito.Mockito.clearInvocations;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.view.View;
import android.widget.RadioGroup;
import app.organicmaps.sdk.Router;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.routing.RoutingOptions;
import app.organicmaps.sdk.settings.RoadType;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
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
    RoutingPlanViewModel viewModel = mock(RoutingPlanViewModel.class);
    View container = mock(View.class);
    View settingsButton = mock(View.class);
    setField(fragment, "mViewModel", viewModel);
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
      options.when(() -> RoutingOptions.getOptions(Router.Vehicle))
          .thenReturn(RoadType.Toll.nativeValue | RoadType.Motorway.nativeValue | RoadType.Dirty.nativeValue);
      options.when(() -> RoutingOptions.getOptions(Router.Bicycle)).thenReturn(0);
      options.when(() -> RoutingOptions.getOptions(Router.Pedestrian)).thenReturn(RoadType.Ferry.nativeValue);
      options.when(() -> RoutingOptions.getOptions(Router.Transit)).thenReturn(0);
      options.when(() -> RoutingOptions.getOptions(Router.Ruler)).thenReturn(0);

      updateProgress(fragment, Router.Vehicle);
      verify(viewModel).setDrivingOptionsCount(3);
      verify(container).setAlpha(1.0f);
      verify(settingsButton).setEnabled(true);
      clearInvocations(viewModel, container, settingsButton);

      updateProgress(fragment, Router.Bicycle);
      verify(viewModel).setDrivingOptionsCount(0);
      verify(container).setAlpha(1.0f);
      verify(settingsButton).setEnabled(true);
      clearInvocations(viewModel, container, settingsButton);

      updateProgress(fragment, Router.Pedestrian);
      verify(viewModel).setDrivingOptionsCount(1);
      verify(container).setAlpha(1.0f);
      verify(settingsButton).setEnabled(true);
      clearInvocations(viewModel, container, settingsButton);

      updateProgress(fragment, Router.Transit);
      verify(viewModel).setDrivingOptionsCount(0);
      verify(container).setAlpha(1.0f);
      verify(settingsButton).setEnabled(true);
      clearInvocations(viewModel, container, settingsButton);

      updateProgress(fragment, Router.Ruler);
      verify(viewModel).setDrivingOptionsCount(0);
      verify(container).setAlpha(0.5f);
      verify(settingsButton).setEnabled(false);
    }
  }

  @Test
  public void aPartialPlanUpdatesItsBadgeWithoutABuildCallback() throws ReflectiveOperationException
  {
    RoutingPlanFragment fragment = mock(RoutingPlanFragment.class, CALLS_REAL_METHODS);
    RoutingPlanViewModel viewModel = mock(RoutingPlanViewModel.class);
    View container = mock(View.class);
    View settingsButton = mock(View.class);
    RadioGroup types = mock(RadioGroup.class);
    View button = mock(View.class);
    when(types.findViewById(1)).thenReturn(button);
    setField(fragment, "mViewModel", viewModel);
    setField(fragment, "mDrivingOptionsContainer", container);
    setField(fragment, "mDrivingOptionsBtn", settingsButton);
    setField(fragment, "mRoutingContentActive", true);
    setField(fragment, "mRouterTypes", types);
    RoutingController controller = mock(RoutingController.class);
    try (MockedStatic<RoutingController> routing = mockStatic(RoutingController.class);
         MockedStatic<RoutingOptions> options = mockStatic(RoutingOptions.class, CALLS_REAL_METHODS))
    {
      routing.when(RoutingController::get).thenReturn(controller);
      options.when(() -> RoutingOptions.getOptions(Router.Bicycle)).thenReturn(0);
      Method install = RoutingPlanFragment.class.getDeclaredMethod("setRouterClick", int.class, Router.class);
      install.setAccessible(true);
      install.invoke(fragment, 1, Router.Bicycle);
      ArgumentCaptor<View.OnClickListener> listener = ArgumentCaptor.forClass(View.OnClickListener.class);
      verify(button).setOnClickListener(listener.capture());
      listener.getValue().onClick(button);
      verify(controller).setRouterType(Router.Bicycle);
      verify(viewModel).setDrivingOptionsCount(0);
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
