package app.organicmaps.sdk.bookmarks.data;

import android.annotation.SuppressLint;
import android.os.Parcel;
import androidx.annotation.ColorInt;
import androidx.annotation.IntRange;
import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.os.ParcelCompat;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.routing.RoutePointInfo;
import app.organicmaps.sdk.search.Popularity;
import app.organicmaps.sdk.util.Constants;

// TODO consider refactoring to remove hack with MapObject unmarshalling itself and Bookmark at the same time.
@SuppressLint("ParcelCreator")
public class Bookmark extends MapObject
{
  private Icon mIcon; // Icon should not be 'final' because its color could be changed.
  private long mCategoryId;
  private final long mBookmarkId;
  private final String mDescription;
  private final double mScale;

  // Used by JNI.
  @Keep
  @SuppressWarnings("unused")
  private Bookmark(@IntRange(from = 0) long categoryId, @IntRange(from = 0) long bookmarkId, String title,
                   @Nullable String secondaryTitle, @Nullable String subtitle, @Nullable String address,
                   @Nullable RoutePointInfo routePointInfo, @OpeningMode int openingMode, @NonNull String wikiArticle,
                   @NonNull String osmDescription, @Nullable String[] rawTypes)
  {
    super(BOOKMARK, title, secondaryTitle, subtitle, address, 0, 0, "", routePointInfo, openingMode, wikiArticle,
          osmDescription, RoadWarningMarkType.UNKNOWN.ordinal(), rawTypes);

    mCategoryId = categoryId;
    mBookmarkId = bookmarkId;
    BookmarkInfo bookmarkInfo = loadBookmarkInfo();
    mDescription = bookmarkInfo.getDescription();
    mIcon = bookmarkInfo.getIcon();
    mScale = bookmarkInfo.getScale();

    setLat(bookmarkInfo.getLat());
    setLon(bookmarkInfo.getLon());
    setTitle(bookmarkInfo.getName());
  }

  @Override
  public void writeToParcel(Parcel dest, int flags)
  {
    super.writeToParcel(dest, flags); // Super class writes bookmark lat, lon and title
    dest.writeLong(mCategoryId);
    dest.writeLong(mBookmarkId);
    dest.writeString(mDescription);
    dest.writeParcelable(mIcon, flags);
    dest.writeDouble(mScale);
  }

  // Do not use Core while restoring from Parcel! In some cases this constructor is called before
  // the App is completely initialized.
  // TODO: Method restoreHasCurrentPermission causes this strange behaviour, needs to be investigated.
  protected Bookmark(@MapObjectType int type, Parcel source)
  {
    super(type, source); // Super class reads bookmark lat, lon and title
    mCategoryId = source.readLong();
    mBookmarkId = source.readLong();
    mDescription = source.readString();
    mIcon = ParcelCompat.readParcelable(source, Icon.class.getClassLoader(), Icon.class);
    mScale = source.readDouble();
  }

  public long getBookmarkId()
  {
    return mBookmarkId;
  }

  @Override
  public double getScale()
  {
    return mScale;
  }

  @Nullable
  public Icon getIcon()
  {
    return mIcon;
  }

  public long getCategoryId()
  {
    return mCategoryId;
  }

  public void setCategoryId(@IntRange(from = 0) long catId)
  {
    if (mCategoryId == catId)
      return;
    nativeChangeCategory(mCategoryId, catId, mBookmarkId);
    mCategoryId = catId;
  }

  public void setIconColor(@ColorInt int color)
  {
    final int colorIndex = PredefinedColors.getPredefinedColorIndex(color);
    mIcon = new Icon(colorIndex, mIcon.getType());
    nativeSetColor(mBookmarkId, colorIndex);
  }

  @PredefinedColors.Color
  public int getColor()
  {
    return mIcon.getColor();
  }

  @NonNull
  public String getDescription()
  {
    return mDescription;
  }

  @NonNull
  private BookmarkInfo loadBookmarkInfo()
  {
    BookmarkInfo info = BookmarkManager.INSTANCE.getBookmarkInfo(mBookmarkId);
    if (info == null)
      throw new IllegalStateException("BookmarkInfo for " + mBookmarkId + " not found.");

    return info;
  }

  static native int nativeSetColor(long bookmarkId, @PredefinedColors.Color int color);

  static native void nativeUpdateParams(long bookmarkId, @NonNull String name, @PredefinedColors.Color int color,
                                        @NonNull String description);
  static native void nativeChangeCategory(long oldCatId, long newCatId, long bookmarkId);

  @NonNull
  private static native String nativeEncode2Ge0Url(long bookmarkId, boolean addName);
}
