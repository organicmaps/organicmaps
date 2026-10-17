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
  public void areasOfferOnlyKmz()
  {
    List<FileType> selected = new ArrayList<>();
    List<MenuBottomSheetItem> items = ExportMenuItems.create(true, selected::add);

    assertEquals(1, items.size());
    assertEquals(R.string.export_file, items.get(0).titleRes);
    items.get(0).onClickListener.onClick();
    assertEquals(List.of(FileType.Kml), selected);
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
