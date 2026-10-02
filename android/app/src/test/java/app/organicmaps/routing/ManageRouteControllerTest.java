package app.organicmaps.routing;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.Mockito.CALLS_REAL_METHODS;
import static org.mockito.Mockito.doCallRealMethod;
import static org.mockito.Mockito.doNothing;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyNoInteractions;
import static org.mockito.Mockito.when;

import android.content.Context;
import android.view.View;
import androidx.recyclerview.widget.ItemTouchHelper;
import androidx.recyclerview.widget.RecyclerView;
import app.organicmaps.sdk.routing.RouteMarkData;
import app.organicmaps.sdk.routing.RouteMarkType;
import app.organicmaps.sdk.routing.RoutingController;
import java.lang.reflect.Constructor;
import java.lang.reflect.Field;
import java.util.ArrayList;
import java.util.List;
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

  private static final class DragFixture
  {
    final ManageRouteAdapter adapter;
    final ManageRouteController controller;
    final RoutingController routing = mock(RoutingController.class);
    final RecyclerView recycler = mock(RecyclerView.class);
    final List<Runnable> posted = new ArrayList<>();
    final ItemTouchHelper.Callback drag;

    DragFixture() throws ReflectiveOperationException
    {
      controller = controller(point(RouteMarkType.Start, 0), point(RouteMarkType.Intermediate, 0),
                              point(RouteMarkType.Finish, 0));
      final Field field = ManageRouteController.class.getDeclaredField("mManageRouteAdapter");
      field.setAccessible(true);
      adapter = spy((ManageRouteAdapter) field.get(controller));
      field.set(controller, adapter);
      doNothing().when(adapter).notifyItemMoved(anyInt(), anyInt());
      doNothing().when(adapter).notifyDataSetChanged();
      doNothing().when(controller).onRouteOrderChanged(any());
      doNothing().when(controller).refresh();
      final Class<?> callback =
          Class.forName(ManageRouteController.class.getName() + "$ManageRouteItemTouchHelperCallback");
      final Constructor<?> constructor =
          callback.getDeclaredConstructor(ManageRouteAdapter.class, ManageRouteController.class);
      constructor.setAccessible(true);
      drag = (ItemTouchHelper.Callback) constructor.newInstance(adapter, controller);
      when(routing.isPlanning()).thenReturn(true);
      when(recycler.isAttachedToWindow()).thenReturn(true);
      when(recycler.post(any())).thenAnswer(call -> {
        posted.add(call.getArgument(0));
        return true;
      });
    }

    RecyclerView.ViewHolder pointHolder(int position)
    {
      final ManageRouteAdapter.ManageRouteViewHolder holder =
          spy(new ManageRouteAdapter.ManageRouteViewHolder(mock(View.class)));
      doReturn(position).when(holder).getBindingAdapterPosition();
      return holder;
    }

    RecyclerView.ViewHolder move()
    {
      final RecyclerView.ViewHolder source = pointHolder(0);
      drag.onSelectedChanged(source, ItemTouchHelper.ACTION_STATE_DRAG);
      assertTrue(drag.onMove(recycler, source, pointHolder(1)));
      doReturn(1).when(source).getBindingAdapterPosition();
      return source;
    }
  }

  @Test
  public void editsStayBlockedThroughDragAndRecovery() throws ReflectiveOperationException
  {
    final DragFixture fixture = new DragFixture();
    try (MockedStatic<RoutingController> singleton = mockStatic(RoutingController.class))
    {
      singleton.when(RoutingController::get).thenReturn(fixture.routing);
      final RecyclerView.ViewHolder source = fixture.move();
      fixture.controller.onRoutePointDeleted(fixture.pointHolder(0));
      fixture.drag.clearView(fixture.recycler, source);
      fixture.controller.onRoutePointDeleted(fixture.pointHolder(0));
      verify(fixture.routing, never()).removeStop(any(), anyInt());
      fixture.posted.get(0).run();
      verify(fixture.controller).onRouteOrderChanged(fixture.adapter.getRoutePoints());
      fixture.controller.onRoutePointDeleted(fixture.pointHolder(0));
      verify(fixture.routing).removeStop(RouteMarkType.Start, 0);
    }
  }

  @Test
  public void refreshDoesNotReplaceThePendingOrder() throws ReflectiveOperationException
  {
    final DragFixture fixture = new DragFixture();
    fixture.move();
    doCallRealMethod().when(fixture.controller).refresh();
    fixture.controller.refresh();
    assertEquals(RouteMarkType.Intermediate.name(), fixture.adapter.getRoutePoints().get(0).mTitle);
  }

  @Test
  public void cancelledOrDestroyedPlannerDiscardsItsQueuedDrop() throws ReflectiveOperationException
  {
    for (boolean attached : new boolean[] {true, false})
    {
      final DragFixture fixture = new DragFixture();
      try (MockedStatic<RoutingController> singleton = mockStatic(RoutingController.class))
      {
        singleton.when(RoutingController::get).thenReturn(fixture.routing);
        fixture.drag.clearView(fixture.recycler, fixture.move());
        when(fixture.recycler.isAttachedToWindow()).thenReturn(attached);
        when(fixture.routing.isPlanning()).thenReturn(!attached);
        fixture.posted.get(0).run();
        verify(fixture.controller, never()).onRouteOrderChanged(any());
        verify(fixture.controller, never()).refresh();
      }
    }
  }

  @Test
  public void regrabbingRecoveryLetsOnlyTheLatestDropCommit() throws ReflectiveOperationException
  {
    final DragFixture fixture = new DragFixture();
    try (MockedStatic<RoutingController> singleton = mockStatic(RoutingController.class))
    {
      singleton.when(RoutingController::get).thenReturn(fixture.routing);
      final RecyclerView.ViewHolder source = fixture.move();
      fixture.drag.clearView(fixture.recycler, source);
      fixture.drag.onSelectedChanged(source, ItemTouchHelper.ACTION_STATE_DRAG);
      fixture.posted.get(0).run();
      verify(fixture.controller, never()).onRouteOrderChanged(any());
      fixture.controller.onRoutePointDeleted(fixture.pointHolder(0));
      verify(fixture.routing, never()).removeStop(any(), anyInt());
      fixture.drag.clearView(fixture.recycler, source);
      fixture.posted.get(1).run();
      verify(fixture.controller).onRouteOrderChanged(fixture.adapter.getRoutePoints());
    }
  }

  @Test
  public void unchangedDropRefreshesWithoutRebuilding() throws ReflectiveOperationException
  {
    final DragFixture fixture = new DragFixture();
    try (MockedStatic<RoutingController> singleton = mockStatic(RoutingController.class))
    {
      singleton.when(RoutingController::get).thenReturn(fixture.routing);
      final RecyclerView.ViewHolder source = fixture.pointHolder(0);
      fixture.drag.onSelectedChanged(source, ItemTouchHelper.ACTION_STATE_DRAG);
      fixture.drag.clearView(fixture.recycler, source);
      fixture.posted.get(0).run();
      verify(fixture.controller, never()).onRouteOrderChanged(any());
      verify(fixture.controller).refresh();
    }
  }

  @Test
  public void invalidDragPositionsDoNotChangeTheOrder() throws ReflectiveOperationException
  {
    final DragFixture fixture = new DragFixture();
    assertFalse(
        fixture.drag.onMove(fixture.recycler, fixture.pointHolder(RecyclerView.NO_POSITION), fixture.pointHolder(1)));
    assertFalse(
        fixture.drag.onMove(fixture.recycler, fixture.pointHolder(0), fixture.pointHolder(RecyclerView.NO_POSITION)));
    assertEquals(RouteMarkType.Start.name(), fixture.adapter.getRoutePoints().get(0).mTitle);
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
