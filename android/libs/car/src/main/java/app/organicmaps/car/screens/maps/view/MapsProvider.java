package app.organicmaps.car.screens.maps.view;

import android.os.ConditionVariable;
import android.util.SparseArray;
import androidx.annotation.DrawableRes;
import androidx.annotation.NonNull;
import androidx.annotation.WorkerThread;
import androidx.car.app.CarContext;
import androidx.car.app.model.CarIcon;
import androidx.core.graphics.drawable.IconCompat;
import app.organicmaps.car.R;
import app.organicmaps.sdk.downloader.CountryItem;
import app.organicmaps.sdk.downloader.MapManager;
import app.organicmaps.sdk.util.concurrency.ThreadPool;
import app.organicmaps.sdk.util.concurrency.UiThread;
import java.util.ArrayList;
import java.util.List;

final class MapsProvider
{
  public interface OnMapsLoadedListener
  {
    @WorkerThread
    void onMapsLoaded(@NonNull List<MapItem> maps);
  }

  public enum Type
  {
    All,
    Mine,
    Updatable
  }

  public record MapItem(@NonNull String id, @NonNull String name, @NonNull String description, boolean browsable,
                        boolean downloadable, boolean removable, int status, int childCount, int totalChildCount)
  {
    // Cache keyed by (status * 2 + (browsable ? 1 : 0)) – covers every (status, type) pair.
    private static final SparseArray<CarIcon> sIconCache = new SparseArray<>();

    /**
     * Returns the status icon for this map item, creating and caching it on first call.
     * Mirrors the logic in {@code DownloaderStatusIcon.selectIcon} and its override in
     * {@code DownloaderAdapter.ItemViewHolder}: folders use folder icons, leaves use map icons.
     *
     * <p>Thread-safe: {@code icon()} may be called from worker threads during row pre-building.
     */
    @NonNull
    public CarIcon icon(@NonNull CarContext context)
    {
      final int cacheKey = status * 2 + (browsable ? 1 : 0);
      synchronized (sIconCache)
      {
        final CarIcon cached = sIconCache.get(cacheKey);
        if (cached != null)
          return cached;

        final CarIcon icon = new CarIcon.Builder(IconCompat.createWithResource(context, selectIconDrawable())).build();
        sIconCache.put(cacheKey, icon);
        return icon;
      }
    }

    public boolean needsDownload()
    {
      return downloadable || (browsable && status != CountryItem.STATUS_DONE);
    }

    public boolean isDownloaded()
    {
      return removable || (browsable && status == CountryItem.STATUS_DONE);
    }

    @DrawableRes
    private int selectIconDrawable()
    {
      if (browsable)
      {
        // Folder fully downloaded → folder_done; anything else → generic folder.
        return (status == CountryItem.STATUS_DONE) ? R.drawable.ic_downloader_folder_done
                                                   : R.drawable.ic_downloader_folder;
      }

      // Leaf map icons match DownloaderStatusIcon.selectIcon().
      return switch (status)
      {
        case CountryItem.STATUS_DONE -> R.drawable.ic_downloader_done;
        case CountryItem.STATUS_UPDATABLE -> R.drawable.ic_downloader_update;
        case CountryItem.STATUS_FAILED -> R.drawable.ic_downloader_failed;
        // DOWNLOADABLE, PARTLY, PROGRESS, APPLYING, ENQUEUED, UNKNOWN → download icon.
        default -> R.drawable.ic_download;
      };
    }
  }

  /**
   * Loads the children of {@code root} according to {@code type} on a worker thread and
   * calls {@code onMapsLoadedListener} with the sorted result.
   * <p>
   * Pass {@link CountryItem#getRootId()} to load the top-level list, or any sub-folder ID
   * obtained from a {@link MapItem#id()} to browse into that folder recursively.
   */
  public static void getMaps(@NonNull String root, @NonNull Type type,
                             @NonNull OnMapsLoadedListener onMapsLoadedListener)
  {
    ThreadPool.getWorker().submit(() -> {
      final List<CountryItem> items = new ArrayList<>();
      final boolean myMaps = type != Type.All;
      final ConditionVariable cv = new ConditionVariable();
      // TODO: Why do we need to run this on UI Thread?
      UiThread.runLater(() -> {
        MapManager.nativeListItems(root, 0, 0, false /* hasLocation */, myMaps, items);
        cv.open();
      });
      cv.block();

      items.sort(CountryItem::compareTo);
      final List<MapItem> maps = new ArrayList<>();
      for (final CountryItem item : items)
      {
        // The Updatable tab shows maps that need an update (STATUS_UPDATABLE), a retry
        // (STATUS_FAILED), or a resumed download (STATUS_PARTLY), plus folders whose
        // aggregate status is STATUS_UPDATABLE. Folders never have STATUS_FAILED or
        // STATUS_PARTLY at the aggregate level, so those conditions apply to leaf maps only.
        if (type == Type.Updatable && item.status != CountryItem.STATUS_UPDATABLE
            && item.status != CountryItem.STATUS_FAILED)
          continue;

        // Updatable/failed maps are treated as downloadable so the row tap triggers a download.
        // When type == Updatable, all remaining items already passed the filter above.
        final boolean isDownloadable = type == Type.Updatable || item.status == CountryItem.STATUS_DOWNLOADABLE
                                    || item.status == CountryItem.STATUS_FAILED;
        maps.add(new MapItem(item.id, item.name, item.description, item.isExpandable(), isDownloadable, item.present,
                             item.status, item.childCount, item.totalChildCount));
      }
      onMapsLoadedListener.onMapsLoaded(maps);
    });
  }
}
