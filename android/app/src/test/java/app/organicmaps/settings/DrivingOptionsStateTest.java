package app.organicmaps.settings;

import static org.mockito.Mockito.CALLS_REAL_METHODS;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import androidx.appcompat.widget.SwitchCompat;
import androidx.fragment.app.FragmentActivity;
import app.organicmaps.R;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.routing.RoutingOptions;
import org.junit.Test;
import org.mockito.MockedStatic;

public class DrivingOptionsStateTest
{
  @Test
  public void recreationRetainsTheBaselineAndReportsAnEditOnce()
  {
    DrivingOptionsFragment fragment = mock(DrivingOptionsFragment.class, CALLS_REAL_METHODS);
    FragmentActivity activity = mock(FragmentActivity.class);
    doReturn(activity).when(fragment).requireActivity();
    LayoutInflater inflater = mock(LayoutInflater.class);
    View root = mock(View.class);
    when(inflater.inflate(R.layout.fragment_driving_options, null, false)).thenReturn(root);
    when(root.findViewById(R.id.content)).thenReturn(mock(View.class));
    for (int id : new int[] {R.id.route_optimization_btn, R.id.avoid_tolls_btn, R.id.avoid_motorways_btn,
                             R.id.avoid_ferries_btn, R.id.avoid_dirty_roads_btn})
      when(root.findViewById(id)).thenReturn(mock(SwitchCompat.class));
    Bundle restored = mock(Bundle.class);
    when(restored.containsKey("road_types_mask")).thenReturn(true);
    when(restored.getInt("road_types_mask")).thenReturn(2);
    RoutingController controller = mock(RoutingController.class);
    try (MockedStatic<RoutingOptions> options = mockStatic(RoutingOptions.class);
         MockedStatic<RoutingController> routing = mockStatic(RoutingController.class))
    {
      options.when(RoutingOptions::getOptions).thenReturn(16);
      routing.when(RoutingController::get).thenReturn(controller);
      fragment.onCreateView(inflater, null, restored);
      Bundle saved = mock(Bundle.class);
      fragment.onSaveInstanceState(saved);
      verify(saved).putInt("road_types_mask", 2);
      fragment.onStop();
      fragment.onStop();
      verify(controller, times(1)).onRoutingOptionsChanged();
      Bundle rebaselined = mock(Bundle.class);
      fragment.onSaveInstanceState(rebaselined);
      verify(rebaselined).putInt("road_types_mask", 16);
    }
  }
}
