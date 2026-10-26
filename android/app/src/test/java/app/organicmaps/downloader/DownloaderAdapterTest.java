package app.organicmaps.downloader;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNull;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

import androidx.fragment.app.FragmentActivity;
import app.organicmaps.R;
import app.organicmaps.sdk.downloader.CountryItem;
import app.organicmaps.util.bottomsheet.MenuBottomSheetItem;
import java.util.List;
import org.junit.Test;

public class DownloaderAdapterTest
{
  /**
   * A selection set on a recreated adapter, as DownloaderFragment does on restore, yields that country's menu.
   * https://github.com/organicmaps/organicmaps/issues/7788
   */
  @Test
  public void restoredSelectionProvidesMenuItems()
  {
    final DownloaderAdapter adapter = newAdapter();
    assertNull(adapter.getSelectedItemId());

    final CountryItem item = new CountryItem("California");
    item.status = CountryItem.STATUS_PARTLY;
    adapter.setSelectedItem(item);

    assertEquals("California", adapter.getSelectedItemId());
    final List<MenuBottomSheetItem> menu = adapter.getMenuItems();
    assertEquals(2, menu.size());
    assertEquals(R.string.downloader_download_map, menu.get(0).titleRes);
    assertEquals(R.string.delete, menu.get(1).titleRes);
  }

  private static DownloaderAdapter newAdapter()
  {
    final DownloaderFragment fragment = mock(DownloaderFragment.class);
    when(fragment.requireActivity()).thenReturn(mock(FragmentActivity.class));
    return new DownloaderAdapter(fragment);
  }
}
