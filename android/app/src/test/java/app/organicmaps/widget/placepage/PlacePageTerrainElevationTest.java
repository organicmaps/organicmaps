package app.organicmaps.widget.placepage;

import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.widget.TextView;
import app.organicmaps.sdk.bookmarks.data.MapObject;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import org.junit.Test;

public class PlacePageTerrainElevationTest
{
  @Test
  public void coordinateFormatsKeepElevationAndASelectionWithoutTerrainClearsIt() throws Exception
  {
    PlacePageView view = mock(PlacePageView.class);
    TextView coordinates = mock(TextView.class);
    setField(view, "mTvLatlon", coordinates);
    MapObject point = mock(MapObject.class);
    when(point.getFormattedTerrainElevation()).thenReturn("125 m");
    setField(view, "mMapObject", point);

    setCoordinates(view, "43.7, 7.2");
    setCoordinates(view, "UTM: 32T 355000 4840000");
    verify(coordinates).setText("43.7, 7.2   ▲125 m");
    verify(coordinates).setText("UTM: 32T 355000 4840000   ▲125 m");

    MapObject nextPoint = mock(MapObject.class);
    when(nextPoint.getFormattedTerrainElevation()).thenReturn("");
    setField(view, "mMapObject", nextPoint);
    setCoordinates(view, "9.5, 138.1");
    verify(coordinates).setText("9.5, 138.1");
  }

  @Test
  public void seaLevelNegativeAndImperialElevationsRemainVisible() throws Exception
  {
    PlacePageView view = mock(PlacePageView.class);
    TextView coordinates = mock(TextView.class);
    setField(view, "mTvLatlon", coordinates);
    MapObject point = mock(MapObject.class);
    setField(view, "mMapObject", point);
    for (String elevation : new String[] {"0 m", "-25 m", "410 ft"})
    {
      when(point.getFormattedTerrainElevation()).thenReturn(elevation);
      setCoordinates(view, "43.7, 7.2");
      verify(coordinates).setText("43.7, 7.2   ▲" + elevation);
    }
  }

  @Test
  public void currentLocationKeepsCoordinatesSeparateFromLiveGpsAltitude() throws Exception
  {
    PlacePageView view = mock(PlacePageView.class);
    TextView coordinates = mock(TextView.class);
    setField(view, "mTvLatlon", coordinates);
    MapObject position = mock(MapObject.class);
    when(position.isMyPosition()).thenReturn(true);
    when(position.getFormattedTerrainElevation()).thenReturn("125 m");
    setField(view, "mMapObject", position);

    setCoordinates(view, "43.7, 7.2");
    verify(coordinates).setText("43.7, 7.2");
  }

  private static void setCoordinates(PlacePageView view, String text) throws Exception
  {
    Method method = PlacePageView.class.getDeclaredMethod("setLatLonText", String.class);
    method.setAccessible(true);
    method.invoke(view, text);
  }

  private static void setField(PlacePageView view, String name, Object value) throws Exception
  {
    Field field = PlacePageView.class.getDeclaredField(name);
    field.setAccessible(true);
    field.set(view, value);
  }
}
