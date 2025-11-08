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
  private Icon mIcon;
  private long mCategoryId;
  private final long mBookmarkId;
  private final double mMerX;
  private final double mMerY;

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
    mIcon = getIconInternal();

    final ParcelablePointD ll = nativeGetXY(mBookmarkId);
    mMerX = ll.x;
    mMerY = ll.y;

    initXY();
  }

  private void initXY()
  {
    setLat(Math.toDegrees(2.0 * Math.atan(Math.exp(Math.toRadians(mMerY))) - Math.PI / 2.0));
    setLon(mMerX);
  }

  @Override
  public void writeToParcel(Parcel dest, int flags)
  {
    super.writeToParcel(dest, flags);
    dest.writeLong(mCategoryId);
    dest.writeLong(mBookmarkId);
    dest.writeParcelable(mIcon, flags);
    dest.writeDouble(mMerX);
    dest.writeDouble(mMerY);
  }

  // Do not use Core while restoring from Parcel! In some cases this constructor is called before
  // the App is completely initialized.
  // TODO: Method restoreHasCurrentPermission causes this strange behaviour, needs to be investigated.
  protected Bookmark(@MapObjectType int type, Parcel source)
  {
    super(type, source);
    mCategoryId = source.readLong();
    mBookmarkId = source.readLong();
    mIcon = ParcelCompat.readParcelable(source, Icon.class.getClassLoader(), Icon.class);
    mMerX = source.readDouble();
    mMerY = source.readDouble();
    initXY();
  }

  public long getBookmarkId()
  {
    return mBookmarkId;
  }

  @Override
  public double getScale()
  {
    return nativeGetScale(mBookmarkId);
  }

  @NonNull
  public DistanceAndAzimut getDistanceAndAzimuth(double cLat, double cLon, double north)
  {
    return Framework.nativeGetDistanceAndAzimuth(mMerX, mMerY, cLat, cLon, north);
  }

  @NonNull
  private Icon getIconInternal()
  {
    return new Icon(getColor(), nativeGetIcon(mBookmarkId));
  }

  @Nullable
  public Icon getIcon()
  {
    return mIcon;
  }

  @NonNull
  public String getCategoryName()
  {
    return BookmarkManager.INSTANCE.getCategoryById(mCategoryId).getName();
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
    mIcon = new Icon(PredefinedColors.getPredefinedColorIndex(color), nativeGetIcon(mBookmarkId));
    nativeSetColor(mBookmarkId, mIcon.getColor());
  }

  @NonNull
  public String getBookmarkFeatureType()
  {
    return nativeGetFeatureType(mBookmarkId);
  }

  @PredefinedColors.Color
  public int getColor()
  {
    return nativeGetColor(mBookmarkId);
  }

  @NonNull
  public String getName()
  {
    return nativeGetName(mBookmarkId);
  }

  @NonNull
  public String getDescription()
  {
    return nativeGetDescription(mBookmarkId);
  }

  @NonNull
  public String getBookmarkAddress()
  {
    return nativeGetAddress(mBookmarkId);
  }

  @NonNull
  public String getGe0Url(boolean addName)
  {
    return nativeEncode2Ge0Url(mBookmarkId, addName);
  }

  @NonNull
  public String getHttpGe0Url(boolean addName)
  {
    return getGe0Url(addName).replaceFirst(Constants.Url.SHORT_SHARE_PREFIX, Constants.Url.HTTP_SHARE_PREFIX);
  }

  @NonNull
  public BookmarkInfo getBookmarkInfo()
  {
    return new BookmarkInfo(mCategoryId, mBookmarkId);
  }

  @NonNull
  static native String nativeGetFeatureType(long bookmarkId);
  @NonNull
  static native String nativeGetName(long bookmarkId);
  @NonNull
  static native String nativeGetDescription(long bookmarkId);
  static native double nativeGetScale(long bookmarkId);
  @NonNull
  static native String nativeGetAddress(long bookmarkId);
  @NonNull
  static native ParcelablePointD nativeGetXY(long bookmarkId);

  @PredefinedColors.Color
  static native int nativeGetColor(long bookmarkId);
  static native int nativeSetColor(long bookmarkId, @PredefinedColors.Color int color);
  static native int nativeGetIcon(long bookmarkId);

  static native void nativeUpdateParams(long bookmarkId, @NonNull String name, @PredefinedColors.Color int color,
                                        @NonNull String description);
  static native void nativeChangeCategory(long oldCatId, long newCatId, long bookmarkId);

  @NonNull
  private static native String nativeEncode2Ge0Url(long bookmarkId, boolean addName);
}
