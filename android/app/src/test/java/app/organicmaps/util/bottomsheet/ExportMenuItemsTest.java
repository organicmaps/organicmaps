package app.organicmaps.util.bottomsheet;

import static org.junit.Assert.assertEquals;

import app.organicmaps.R;
import app.organicmaps.sdk.bookmarks.data.FileType;
import java.util.ArrayList;
import java.util.List;
import org.junit.Test;

public class ExportMenuItemsTest
{
  @Test
  public void areasOfferKmzAndGeoJson()
  {
    List<FileType> selected = new ArrayList<>();
    List<MenuBottomSheetItem> items = ExportMenuItems.create(true, selected::add);

    assertEquals(2, items.size());
    assertEquals(R.string.export_file, items.get(0).titleRes);
    assertEquals(R.string.export_file_geojson, items.get(1).titleRes);
    for (MenuBottomSheetItem item : items)
      item.onClickListener.onClick();
    assertEquals(List.of(FileType.Kml, FileType.GeoJson), selected);
  }

  @Test
  public void tracksAndAreaFreeCategoriesKeepAllFormats()
  {
    List<FileType> selected = new ArrayList<>();
    List<MenuBottomSheetItem> items = ExportMenuItems.create(selected::add);

    for (MenuBottomSheetItem item : items)
      item.onClickListener.onClick();
    assertEquals(List.of(FileType.Kml, FileType.Gpx, FileType.GeoJson), selected);
  }
}
