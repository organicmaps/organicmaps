package app.organicmaps.sdk.bookmarks.data;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

import android.os.Parcel;
import java.lang.reflect.Field;
import java.util.ArrayList;
import java.util.List;
import org.junit.Test;

public class MapObjectTest
{
  @Test
  public void terrainElevationSurvivesParcelWithoutChangingSelectionIdentity() throws Exception
  {
    MapObject point = MapObject.createMapObject(MapObject.POI, "Point", "", 43.7, 7.2);
    assertEquals("", point.getFormattedTerrainElevation());
    Field elevation = MapObject.class.getDeclaredField("mFormattedTerrainElevation");
    elevation.setAccessible(true);
    elevation.set(point, "125 m");
    assertTrue(point.sameAs(MapObject.createMapObject(MapObject.POI, "Point", "", 43.7, 7.2)));

    Parcel parcel = mock(Parcel.class);
    List<String> strings = new ArrayList<>();
    doAnswer(call -> {
      strings.add(call.getArgument(0));
      return null;
    })
        .when(parcel)
        .writeString(any());
    point.writeToParcel(parcel, 0);
    var values = strings.iterator();
    when(parcel.readString()).thenAnswer(call -> values.next());
    when(parcel.readDouble()).thenReturn(43.7, 7.2);

    MapObject restored = MapObject.CREATOR.createFromParcel(parcel);
    assertEquals("125 m", restored.getFormattedTerrainElevation());
    assertEquals(point.getTitle(), restored.getTitle());
    assertEquals(point.getLat(), restored.getLat(), 0);
    assertEquals(point.getLon(), restored.getLon(), 0);
  }
}
