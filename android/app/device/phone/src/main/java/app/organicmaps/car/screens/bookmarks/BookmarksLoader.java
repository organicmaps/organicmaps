package app.organicmaps.car.screens.bookmarks;

import android.graphics.drawable.Drawable;
import android.location.Location;
import android.text.SpannableStringBuilder;
import android.text.Spanned;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.car.app.CarContext;
import androidx.car.app.constraints.ConstraintManager;
import androidx.car.app.model.CarIcon;
import androidx.car.app.model.DistanceSpan;
import androidx.car.app.model.ForegroundCarColorSpan;
import androidx.car.app.model.ItemList;
import androidx.car.app.model.Row;
import androidx.core.graphics.drawable.IconCompat;
import app.organicmaps.MwmApplication;
import app.organicmaps.R;
import app.organicmaps.car.util.Colors;
import app.organicmaps.car.util.RoutingHelpers;
import app.organicmaps.sdk.bookmarks.data.BookmarkCategory;
import app.organicmaps.sdk.bookmarks.data.BookmarkInfo;
import app.organicmaps.sdk.bookmarks.data.BookmarkManager;
import app.organicmaps.sdk.bookmarks.data.Icon;
import app.organicmaps.sdk.bookmarks.data.SortedBlock;
import app.organicmaps.sdk.util.Distance;
import app.organicmaps.sdk.util.concurrency.ThreadPool;
import app.organicmaps.sdk.util.concurrency.UiThread;
import app.organicmaps.util.Graphics;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.concurrent.Future;

class BookmarksLoader implements BookmarkManager.BookmarksSortingListener
{
  public interface OnBookmarksLoaded
  {
    void onBookmarksLoaded(@NonNull ItemList bookmarks);
  }

  // The maximum size should be equal to ConstraintManager.CONTENT_LIMIT_TYPE_LIST.
  // However, having more than 50 items results in android.os.TransactionTooLargeException.
  // This exception occurs because the data parcel size is too large to be transferred between services.
  // The primary cause of this issue is the icons. Even though we have the maximum Icon.TYPE_ICONS.length icons,
  // each row contains a unique icon, resulting in serialization of each icon.
  private static final int MAX_BOOKMARKS_SIZE = 50;

  @Nullable
  private Future<?> mBookmarkLoaderTask = null;

  @NonNull
  private final CarContext mCarContext;

  @NonNull
  private final OnBookmarksLoaded mOnBookmarksLoaded;

  private final long mBookmarkCategoryId;
  private final int mBookmarksListSize;

  public BookmarksLoader(@NonNull CarContext carContext, @NonNull BookmarkCategory bookmarkCategory,
                         @NonNull OnBookmarksLoaded onBookmarksLoaded)
  {
    final ConstraintManager constraintManager = carContext.getCarService(ConstraintManager.class);
    final int maxCategoriesSize = constraintManager.getContentLimit(ConstraintManager.CONTENT_LIMIT_TYPE_LIST);

    mCarContext = carContext;
    mOnBookmarksLoaded = onBookmarksLoaded;
    mBookmarkCategoryId = bookmarkCategory.getId();
    mBookmarksListSize =
        Math.min(bookmarkCategory.getBookmarksCount(), Math.min(maxCategoriesSize, MAX_BOOKMARKS_SIZE));
  }

  public void load()
  {
    UiThread.runLater(() -> {
      BookmarkManager.INSTANCE.addSortingListener(this);
      if (sortBookmarks())
        return;

      final List<Long> bookmarkIds = new ArrayList<>();
      for (int i = 0; i < mBookmarksListSize; ++i)
        bookmarkIds.add(BookmarkManager.INSTANCE.getBookmarkIdByPosition(mBookmarkCategoryId, i));
      loadBookmarks(bookmarkIds);
    });
  }

  public void cancel()
  {
    BookmarkManager.INSTANCE.removeSortingListener(this);
    if (mBookmarkLoaderTask != null)
    {
      mBookmarkLoaderTask.cancel(true);
      mBookmarkLoaderTask = null;
    }
  }

  /**
   * Calls BookmarkManager to sort bookmarks.
   *
   * @return false if the sorting not needed or can't be done.
   */
  private boolean sortBookmarks()
  {
    if (!BookmarkManager.INSTANCE.hasLastSortingType(mBookmarkCategoryId))
      return false;

    final int sortingType = BookmarkManager.INSTANCE.getLastSortingType(mBookmarkCategoryId);
    if (sortingType < 0)
      return false;

    final Location loc = MwmApplication.from(mCarContext).getLocationHelper().getSavedLocation();
    final boolean hasMyPosition = loc != null;
    if (!hasMyPosition && sortingType == BookmarkManager.SORT_BY_DISTANCE)
      return false;

    final double lat = hasMyPosition ? loc.getLatitude() : 0;
    final double lon = hasMyPosition ? loc.getLongitude() : 0;

    BookmarkManager.INSTANCE.getSortedCategory(mBookmarkCategoryId, sortingType, hasMyPosition, lat, lon, 0);

    return true;
  }

  private void loadBookmarks(@NonNull List<Long> bookmarksIds)
  {
    final BookmarkInfo[] bookmarks = new BookmarkInfo[mBookmarksListSize];
    for (int i = 0; i < mBookmarksListSize && i < bookmarksIds.size(); ++i)
    {
      final long id = bookmarksIds.get(i);
      bookmarks[i] = new BookmarkInfo(mBookmarkCategoryId, id);
    }

    mBookmarkLoaderTask = ThreadPool.getWorker().submit(() -> {
      final ItemList bookmarksList = createBookmarksList(bookmarks);
      UiThread.run(() -> {
        cancel();
        mOnBookmarksLoaded.onBookmarksLoaded(bookmarksList);
      });
    });
  }

  @NonNull
  private ItemList createBookmarksList(@NonNull BookmarkInfo[] bookmarks)
  {
    final Location location = MwmApplication.from(mCarContext).getLocationHelper().getSavedLocation();
    final ItemList.Builder builder = new ItemList.Builder();
    final Map<Icon, CarIcon> iconsCache = new HashMap<>();
    for (final BookmarkInfo bookmarkInfo : bookmarks)
    {
      final Row.Builder itemBuilder = new Row.Builder();
      itemBuilder.setTitle(bookmarkInfo.getName());
      if (!bookmarkInfo.getAddress().isEmpty())
        itemBuilder.addText(bookmarkInfo.getAddress());
      final CharSequence description = getDescription(bookmarkInfo, location);
      if (description.length() != 0)
        itemBuilder.addText(description);
      final Icon icon = bookmarkInfo.getIcon();
      if (!iconsCache.containsKey(icon))
      {
        final Drawable drawable = Graphics.drawCircleAndImage(icon.argb(), R.dimen.track_circle_size, icon.getResId(),
                                                              R.dimen.bookmark_icon_size, mCarContext);
        final CarIcon carIcon =
            new CarIcon.Builder(IconCompat.createWithBitmap(Graphics.drawableToBitmap(drawable))).build();
        iconsCache.put(icon, carIcon);
      }
      itemBuilder.setImage(Objects.requireNonNull(iconsCache.get(icon)));
      itemBuilder.setOnClickListener(() -> BookmarkManager.INSTANCE.showBookmarkOnMap(bookmarkInfo.getBookmarkId()));
      builder.addItem(itemBuilder.build());
    }
    return builder.build();
  }

  @NonNull
  private static CharSequence getDescription(@NonNull BookmarkInfo bookmark, @Nullable Location location)
  {
    final SpannableStringBuilder result = new SpannableStringBuilder("");
    if (location != null)
    {
      result.append(" ");
      final Distance distance = bookmark.getDistance(location.getLatitude(), location.getLongitude(), 0.0);
      result.setSpan(DistanceSpan.create(RoutingHelpers.createDistance(distance)), 0, 1,
                     Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
      result.setSpan(ForegroundCarColorSpan.create(Colors.DISTANCE), 0, 1, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
    }

    if (!bookmark.getFeatureType().isEmpty())
    {
      if (result.length() > 0)
        result.append(" • ");
      result.append(bookmark.getFeatureType());
    }

    return result;
  }

  @Override
  public void onBookmarksSortingCompleted(@NonNull SortedBlock[] sortedBlocks, long timestamp)
  {
    final List<Long> bookmarkIds = new ArrayList<>();
    for (final SortedBlock block : sortedBlocks)
      bookmarkIds.addAll(block.getBookmarkIds());
    loadBookmarks(bookmarkIds);
  }
}
