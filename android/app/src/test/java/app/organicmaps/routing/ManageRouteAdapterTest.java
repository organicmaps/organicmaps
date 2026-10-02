package app.organicmaps.routing;

import static org.junit.Assert.assertEquals;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.Mockito.doNothing;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.content.Context;
import android.content.res.Resources;
import android.content.res.TypedArray;
import android.view.View;
import android.widget.ImageView;
import android.widget.TextView;
import androidx.appcompat.content.res.AppCompatResources;
import androidx.recyclerview.widget.RecyclerView;
import app.organicmaps.R;
import app.organicmaps.sdk.routing.RouteMarkData;
import app.organicmaps.sdk.routing.RouteMarkType;
import app.organicmaps.util.ThemeUtils;
import java.util.List;
import org.junit.Test;
import org.mockito.MockedStatic;

public class ManageRouteAdapterTest
{
  private static RouteMarkData point(String title, RouteMarkType type, int index)
  {
    return new RouteMarkData(title, "", type, index, true, false, false, index, index);
  }

  @Test
  public void nonAdjacentMovesPreserveTheInterveningOrderAndRenumberStops()
  {
    final int[][] moves = {{0, 3}, {3, 0}, {1, 3}, {3, 1}};
    final List<List<String>> orders = List.of(List.of("B", "C", "D", "A"), List.of("D", "A", "B", "C"),
                                              List.of("A", "C", "D", "B"), List.of("A", "D", "B", "C"));
    for (int caseIndex = 0; caseIndex < moves.length; ++caseIndex)
    {
      final int[] move = moves[caseIndex];
      final RouteMarkData[] points = {point("A", RouteMarkType.Start, 0), point("B", RouteMarkType.Intermediate, 0),
                                      point("C", RouteMarkType.Intermediate, 1), point("D", RouteMarkType.Finish, 0)};
      final ManageRouteAdapter adapter =
          spy(new ManageRouteAdapter(mock(Context.class), points, mock(ManageRouteAdapter.ManageRouteListener.class)));
      doNothing().when(adapter).notifyItemMoved(anyInt(), anyInt());

      final RecyclerView.ViewHolder source = mock(RecyclerView.ViewHolder.class);
      final RecyclerView.ViewHolder target = mock(RecyclerView.ViewHolder.class);
      when(source.getBindingAdapterPosition()).thenReturn(move[0]);
      when(target.getBindingAdapterPosition()).thenReturn(move[1]);
      adapter.moveRoutePoint(source, target);

      assertEquals(orders.get(caseIndex), adapter.getRoutePoints().stream().map(p -> p.mTitle).toList());
      assertEquals(RouteMarkType.Start, adapter.getRoutePoints().get(0).mPointType);
      assertEquals(RouteMarkType.Finish, adapter.getRoutePoints().get(3).mPointType);
      for (int index = 1; index < 3; ++index)
      {
        assertEquals(RouteMarkType.Intermediate, adapter.getRoutePoints().get(index).mPointType);
        assertEquals(index - 1, adapter.getRoutePoints().get(index).mIntermediateIndex);
      }
      verify(adapter).notifyItemMoved(move[0], move[1]);
    }
  }

  @Test
  public void allSupportedStopIndicesBindWithoutReadingBeyondTheIconArray()
  {
    final Context context = mock(Context.class);
    final Resources resources = mock(Resources.class);
    when(context.getResources()).thenReturn(resources);
    final TypedArray icons = mock(TypedArray.class);
    when(icons.length()).thenReturn(20);
    when(resources.obtainTypedArray(R.array.route_stop_icons)).thenReturn(icons);
    when(icons.getResourceId(anyInt(), anyInt())).thenAnswer(call -> {
      final int index = call.getArgument(0);
      if (index < 0 || index >= 20)
        throw new ArrayIndexOutOfBoundsException(index);
      return index == 19 ? R.drawable.route_point_20 : R.drawable.route_point_01;
    });
    final View row = mock(View.class);
    when(row.findViewById(R.id.type_icon)).thenReturn(mock(ImageView.class));
    when(row.findViewById(R.id.title)).thenReturn(mock(TextView.class));
    when(row.findViewById(R.id.delete_icon)).thenReturn(mock(ImageView.class));
    when(row.findViewById(R.id.drag_icon)).thenReturn(mock(ImageView.class));
    final ManageRouteAdapter.ManageRouteViewHolder holder = new ManageRouteAdapter.ManageRouteViewHolder(row);
    final RouteMarkData[] points = new RouteMarkData[102];
    points[0] = point("Start", RouteMarkType.Start, 0);
    for (int i = 1; i <= 100; ++i)
      points[i] = point("Stop " + i, RouteMarkType.Intermediate, i - 1);
    points[101] = point("Finish", RouteMarkType.Finish, 0);
    final ManageRouteAdapter adapter =
        new ManageRouteAdapter(context, points, mock(ManageRouteAdapter.ManageRouteListener.class));

    try (MockedStatic<ThemeUtils> theme = mockStatic(ThemeUtils.class);
         MockedStatic<AppCompatResources> drawables = mockStatic(AppCompatResources.class))
    {
      for (int index = 0; index < 100; ++index)
      {
        drawables.clearInvocations();
        adapter.onBindViewHolder(holder, index + 1);
        final int expected = index >= 19 ? R.drawable.route_point_20 : R.drawable.route_point_01;
        drawables.verify(() -> AppCompatResources.getDrawable(context, expected));
      }
      verify(icons, times(100)).recycle();
    }
  }
}
