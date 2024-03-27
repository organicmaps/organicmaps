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

import app.organicmaps.R;
import app.organicmaps.bookmarks.data.BookmarkCategory;
import app.organicmaps.bookmarks.data.BookmarkInfo;
import app.organicmaps.bookmarks.data.BookmarkManager;
import app.organicmaps.bookmarks.data.Icon;
import app.organicmaps.car.util.Colors;
import app.organicmaps.car.util.RoutingHelpers;
import app.organicmaps.location.LocationHelper;
import app.organicmaps.util.Distance;
import app.organicmaps.util.Graphics;
import app.organicmaps.util.concurrency.ThreadPool;
import app.organicmaps.util.concurrency.UiThread;

import java.util.HashMap;
import java.util.Map;
import java.util.Objects;

public class BookmarksLoader
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

  public static void load(@NonNull CarContext carContext, @NonNull BookmarkCategory bookmarkCategory, @NonNull OnBookmarksLoaded onBookmarksLoaded)
  {
    UiThread.run(() -> {
      final ConstraintManager constraintManager = carContext.getCarService(ConstraintManager.class);
      final int maxCategoriesSize = constraintManager.getContentLimit(ConstraintManager.CONTENT_LIMIT_TYPE_LIST);
      final long bookmarkCategoryId = bookmarkCategory.getId();
      final int bookmarkCategoriesSize = Math.min(bookmarkCategory.getBookmarksCount(), Math.min(maxCategoriesSize, MAX_BOOKMARKS_SIZE));

      final BookmarkInfo[] bookmarks = new BookmarkInfo[bookmarkCategoriesSize];
      for (int i = 0; i < bookmarkCategoriesSize; ++i)
      {
        final long id = BookmarkManager.INSTANCE.getBookmarkIdByPosition(bookmarkCategoryId, i);
        bookmarks[i] = new BookmarkInfo(bookmarkCategoryId, id);
      }

      ThreadPool.getWorker().submit(() -> {
        final ItemList bookmarksList = createBookmarksList(carContext, bookmarks);
        UiThread.run(() -> onBookmarksLoaded.onBookmarksLoaded(bookmarksList));
      });
    });
  }

  @NonNull
  private static ItemList createBookmarksList(@NonNull CarContext carContext, @NonNull BookmarkInfo[] bookmarks)
  {
    final Location location = LocationHelper.from(carContext).getSavedLocation();
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
        final Drawable drawable = Graphics.drawCircleAndImage(icon.argb(),
            R.dimen.track_circle_size,
            icon.getResId(),
            R.dimen.bookmark_icon_size,
            carContext);
        final CarIcon carIcon = new CarIcon.Builder(IconCompat.createWithBitmap(Graphics.drawableToBitmap(drawable))).build();
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
    final SpannableStringBuilder result = new SpannableStringBuilder(" ");
    if (location != null)
    {
      final Distance distance = bookmark.getDistance(location.getLatitude(), location.getLongitude(), 0.0);
      result.setSpan(DistanceSpan.create(RoutingHelpers.createDistance(distance)), 0, 1, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
      result.setSpan(ForegroundCarColorSpan.create(Colors.DISTANCE), 0, 1, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);

      if (!bookmark.getFeatureType().isEmpty())
      {
        result.append(" • ");
        result.append(bookmark.getFeatureType());
      }
    }

    return result;
  }
}
