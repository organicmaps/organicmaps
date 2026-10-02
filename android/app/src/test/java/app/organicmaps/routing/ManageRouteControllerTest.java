package app.organicmaps.routing;

import static org.mockito.Mockito.CALLS_REAL_METHODS;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyNoInteractions;
import static org.mockito.Mockito.when;

import android.content.Context;
import androidx.recyclerview.widget.RecyclerView;
import app.organicmaps.sdk.routing.RouteMarkData;
import app.organicmaps.sdk.routing.RouteMarkType;
import app.organicmaps.sdk.routing.RoutingController;
import java.lang.reflect.Field;
import org.junit.Test;
import org.mockito.MockedStatic;

public class ManageRouteControllerTest
{
  private static RouteMarkData point(RouteMarkType type, int index)
  {
    return new RouteMarkData(type.name(), "", type, index, true, false, false, index, index);
  }

  private static ManageRouteController controller(RouteMarkData... points) throws ReflectiveOperationException
  {
    final ManageRouteController controller = mock(ManageRouteController.class, CALLS_REAL_METHODS);
    final Field adapter = ManageRouteController.class.getDeclaredField("mManageRouteAdapter");
    adapter.setAccessible(true);
    adapter.set(controller, new ManageRouteAdapter(mock(Context.class), points,
                                                   mock(ManageRouteAdapter.ManageRouteListener.class)));
    return controller;
  }

  private static RecyclerView.ViewHolder holder(int position)
  {
    final RecyclerView.ViewHolder holder = mock(RecyclerView.ViewHolder.class);
    when(holder.getBindingAdapterPosition()).thenReturn(position);
    return holder;
  }

  @Test
  public void invalidDeletePositionsDoNotReachTheRoutingController() throws ReflectiveOperationException
  {
    final RoutingController routing = mock(RoutingController.class);
    try (MockedStatic<RoutingController> singleton = mockStatic(RoutingController.class))
    {
      singleton.when(RoutingController::get).thenReturn(routing);
      controller(point(RouteMarkType.Start, 0)).onRoutePointDeleted(holder(RecyclerView.NO_POSITION));
      controller(point(RouteMarkType.Start, 0), point(RouteMarkType.Finish, 0))
          .onRoutePointDeleted(holder(RecyclerView.NO_POSITION));
      verifyNoInteractions(routing);
    }
  }

  @Test
  public void bothPartialSlotsDeleteTheirOnlyRealPoint() throws ReflectiveOperationException
  {
    final RoutingController routing = mock(RoutingController.class);
    try (MockedStatic<RoutingController> singleton = mockStatic(RoutingController.class))
    {
      singleton.when(RoutingController::get).thenReturn(routing);
      controller(point(RouteMarkType.Start, 0)).onRoutePointDeleted(holder(0));
      controller(point(RouteMarkType.Finish, 0)).onRoutePointDeleted(holder(1));
      verify(routing).removeStop(RouteMarkType.Start, 0);
      verify(routing).removeStop(RouteMarkType.Finish, 0);
    }
  }

  @Test
  public void completeRouteDeletionUsesThePointTypeAndIntermediateIndex() throws ReflectiveOperationException
  {
    final RoutingController routing = mock(RoutingController.class);
    final ManageRouteController controller =
        controller(point(RouteMarkType.Start, 0), point(RouteMarkType.Intermediate, 0),
                   point(RouteMarkType.Intermediate, 1), point(RouteMarkType.Finish, 0));
    try (MockedStatic<RoutingController> singleton = mockStatic(RoutingController.class))
    {
      singleton.when(RoutingController::get).thenReturn(routing);
      controller.onRoutePointDeleted(holder(2));
      verify(routing).removeStop(RouteMarkType.Intermediate, 1);
    }
  }
}
