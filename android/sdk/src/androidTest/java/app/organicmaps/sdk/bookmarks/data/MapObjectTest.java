package app.organicmaps.sdk.bookmarks.data;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import android.os.Parcel;
import java.lang.reflect.Field;
import org.junit.Test;

public class MapObjectTest
{
  @Test
  public void previewSubtitleSurvivesParcelWithoutChangingSelectionIdentity() throws Exception
  {
    MapObject point = MapObject.createMapObject(MapObject.POI, "Point", "", 43.7, 7.2);
    assertEquals("", point.getPreviewSubtitle());
    Field previewSubtitle = MapObject.class.getDeclaredField("mPreviewSubtitle");
    previewSubtitle.setAccessible(true);
    previewSubtitle.set(point, "43.7, 7.2 • ▲125 m");
    assertTrue(point.sameAs(MapObject.createMapObject(MapObject.POI, "Point", "", 43.7, 7.2)));

    // A real Parcel checks the read order against the write order across all the value types.
    Parcel parcel = Parcel.obtain();
    try
    {
      point.writeToParcel(parcel, 0);
      parcel.setDataPosition(0);
      MapObject restored = MapObject.CREATOR.createFromParcel(parcel);
      assertEquals(0, parcel.dataAvail());
      assertEquals("43.7, 7.2 • ▲125 m", restored.getPreviewSubtitle());
      assertEquals(point.getTitle(), restored.getTitle());
      assertEquals(point.getLat(), restored.getLat(), 0);
      assertEquals(point.getLon(), restored.getLon(), 0);
    }
    finally
    {
      parcel.recycle();
    }
  }
}
