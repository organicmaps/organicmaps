package app.organicmaps.routing;

import static org.mockito.Mockito.CALLS_REAL_METHODS;
import static org.mockito.Mockito.clearInvocations;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyNoInteractions;
import static org.mockito.Mockito.when;

import android.app.Activity;
import android.view.View;
import android.widget.TextView;
import androidx.recyclerview.widget.RecyclerView;
import app.organicmaps.R;
import app.organicmaps.sdk.routing.TransitRouteInfo;
import app.organicmaps.sdk.routing.TransitStepInfo;
import app.organicmaps.util.Utils;
import app.organicmaps.widget.RoutingProgressButton;
import java.lang.reflect.Field;
import java.util.List;
import org.junit.Test;
import org.mockito.MockedStatic;

public class RoutingTransitSummaryTest
{
  @Test
  public void unavailableSummaryHidesPreviousDetailsWithoutDisplayingZeroTime() throws ReflectiveOperationException
  {
    Fixture fixture = new Fixture();
    TransitRouteInfo available = mock(TransitRouteInfo.class);
    when(available.getTransitSteps()).thenReturn(List.of(mock(TransitStepInfo.class)));
    TransitRouteInfo unavailable = mock(TransitRouteInfo.class);
    when(unavailable.getTransitSteps()).thenReturn(List.of());
    try (MockedStatic<Utils> utils = mockStatic(Utils.class))
    {
      fixture.controller.showTransitInfo(available);
      verify(fixture.transitTime).setVisibility(View.VISIBLE);
      verify(fixture.steps).setVisibility(View.VISIBLE);
      clearInvocations(fixture.transitTime, fixture.steps, fixture.totalTime, fixture.adapter);

      fixture.controller.showTransitInfo(unavailable);
      verify(fixture.transitTime).setVisibility(View.GONE);
      verify(fixture.transitTime, never()).setVisibility(View.VISIBLE);
      verify(fixture.steps).setVisibility(View.GONE);
      verify(fixture.adapter).setItems(List.of());
      verifyNoInteractions(fixture.totalTime);
    }
  }

  @Test
  public void validSummaryCanHaveZeroTime() throws ReflectiveOperationException
  {
    Fixture fixture = new Fixture();
    TransitRouteInfo info = mock(TransitRouteInfo.class);
    when(info.getTransitSteps()).thenReturn(List.of(mock(TransitStepInfo.class)));
    when(info.getTotalTime()).thenReturn(0);
    try (MockedStatic<Utils> utils = mockStatic(Utils.class))
    {
      fixture.controller.showTransitInfo(info);
      verify(fixture.transitTime).setVisibility(View.VISIBLE);
      verify(fixture.steps).setVisibility(View.VISIBLE);
      verify(fixture.adapter).setItems(info.getTransitSteps());
    }
  }

  private static class Fixture
  {
    final RoutingBottomMenuController controller = mock(RoutingBottomMenuController.class, CALLS_REAL_METHODS);
    final View transitTime = mock(View.class);
    final RecyclerView steps = mock(RecyclerView.class);
    final TransitStepAdapter adapter = mock(TransitStepAdapter.class);
    final TextView totalTime = mock(TextView.class);

    Fixture() throws ReflectiveOperationException
    {
      setField("mContext", mock(Activity.class));
      setField("mTransitTime", transitTime);
      setField("mTransitRecyclerView", steps);
      setField("mTransitAdapter", adapter);
      setField("mStart", mock(RoutingProgressButton.class));
      for (String name : List.of("mAltitudeChart", "mError", "mTimeElevationLine", "mTimeVehicle", "mTimeRuler"))
        setField(name, mock(TextView.class));
      View frame = mock(View.class);
      when(frame.findViewById(R.id.transit_time)).thenReturn(transitTime);
      when(frame.findViewById(R.id.total_time)).thenReturn(totalTime);
      when(frame.findViewById(R.id.total_distance)).thenReturn(mock(TextView.class));
      when(frame.findViewById(R.id.dot)).thenReturn(mock(View.class));
      when(frame.findViewById(R.id.pedestrian_icon)).thenReturn(mock(View.class));
      setField("mAltitudeChartFrame", frame);
    }

    private void setField(String name, Object value) throws ReflectiveOperationException
    {
      Field field = RoutingBottomMenuController.class.getDeclaredField(name);
      field.setAccessible(true);
      field.set(controller, value);
    }
  }
}
