package app.organicmaps.routing;

import static org.mockito.Mockito.CALLS_REAL_METHODS;
import static org.mockito.Mockito.clearInvocations;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.view.View;
import android.view.ViewGroup;
import android.widget.RadioGroup;
import app.organicmaps.sdk.Router;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.routing.RoutingOptions;
import app.organicmaps.sdk.settings.RoadType;
import com.google.android.material.bottomsheet.BottomSheetBehavior;
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
      options.when(() -> RoutingOptions.getActiveRoadTypes(Router.Vehicle))
          .thenReturn(EnumSet.of(RoadType.Toll, RoadType.Motorway, RoadType.Dirty));
      options.when(() -> RoutingOptions.getActiveRoadTypes(Router.Bicycle)).thenReturn(EnumSet.noneOf(RoadType.class));
      options.when(() -> RoutingOptions.getActiveRoadTypes(Router.Pedestrian)).thenReturn(EnumSet.of(RoadType.Ferry));
      options.when(() -> RoutingOptions.getActiveRoadTypes(Router.Transit)).thenReturn(EnumSet.noneOf(RoadType.class));
      options.when(() -> RoutingOptions.getActiveRoadTypes(Router.Ruler)).thenReturn(EnumSet.noneOf(RoadType.class));

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
      options.when(() -> RoutingOptions.getActiveRoadTypes(Router.Bicycle)).thenReturn(EnumSet.noneOf(RoadType.class));
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

  @Test
  public void collapsedSheetKeepsTheSummaryAboveTheNestedSteps() throws ReflectiveOperationException
  {
    RoutingPlanFragment fragment = mock(RoutingPlanFragment.class, CALLS_REAL_METHODS);
    ViewGroup chartPanel = mock(ViewGroup.class);
    ViewGroup summaryTile = mock(ViewGroup.class);
    View steps = mock(View.class);
    View routingTypes = mock(View.class);
    @SuppressWarnings("unchecked")
    BottomSheetBehavior<View> sheetBehavior = mock(BottomSheetBehavior.class);
    when(steps.getVisibility()).thenReturn(View.VISIBLE);
    when(steps.getTop()).thenReturn(40);
    when(steps.getParent()).thenReturn(summaryTile);
    when(summaryTile.getTop()).thenReturn(200);
    when(summaryTile.getParent()).thenReturn(chartPanel);
    when(routingTypes.getHeight()).thenReturn(50);
    setField(fragment, "mFrame", mock(View.class));
    setField(fragment, "mChartPanel", chartPanel);
    setField(fragment, "mTransitStepsView", steps);
    setField(fragment, "mRoutingTypesContainer", routingTypes);
    setField(fragment, "mSheetBehavior", sheetBehavior);
    setField(fragment, "mBottomButtonsMaxHeight", 60);
    setField(fragment, "mPeekHeightMargins", 10);

    Method method = RoutingPlanFragment.class.getDeclaredMethod("updateSheetHeights");
    method.setAccessible(true);
    method.invoke(fragment);

    // The steps' top inside the panel is the tile's offset plus the steps' offset within the tile.
    verify(sheetBehavior).setPeekHeight(50 + 200 + 40 + 60 + 10);
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
