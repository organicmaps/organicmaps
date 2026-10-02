package app.organicmaps.routing;

import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;
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
import app.organicmaps.R;
import app.organicmaps.sdk.routing.RouteMarkData;
import app.organicmaps.sdk.routing.RouteMarkType;
import app.organicmaps.util.ThemeUtils;
import org.junit.Test;
import org.mockito.MockedStatic;

public class ManageRouteAdapterTest
{
  private static RouteMarkData point(String title, RouteMarkType type, int index)
  {
    return new RouteMarkData(title, "", type, index, true, false, false, index, index);
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
