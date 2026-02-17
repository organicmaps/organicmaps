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
  private Icon mIcon; // Icon should not be 'final' because it's color could be changed.
  private long mCategoryId;
  private final long mBookmarkId;
  private final BookmarkInfo mBookmarkInfo;
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
    mBookmarkInfo = getBookmarkInfo();
    mIcon = mBookmarkInfo.getIcon();
    mMerX = mBookmarkInfo.getLat();
    mMerY = mBookmarkInfo.getLon();
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
    mBookmarkInfo = getBookmarkInfo();
    mIcon = mBookmarkInfo.getIcon();
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
    return mBookmarkInfo.getScale();
  }

  @NonNull
  public DistanceAndAzimut getDistanceAndAzimuth(double cLat, double cLon, double north)
  {
    return Framework.nativeGetDistanceAndAzimuth(mMerX, mMerY, cLat, cLon, north);
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
    final int colorIndex = PredefinedColors.getPredefinedColorIndex(color);
    mIcon = new Icon(colorIndex, mIcon.getType());
    nativeSetColor(mBookmarkId, colorIndex);
  }

  @NonNull
  public String getBookmarkFeatureType()
  {
    return mBookmarkInfo.getFeatureType();
  }

  @PredefinedColors.Color
  public int getColor()
  {
    return mIcon.getColor();
  }

  @NonNull
  public String getName()
  {
    return mBookmarkInfo.getName();
  }

  @NonNull
  public String getDescription()
  {
    return mBookmarkInfo.getDescription();
  }

  @NonNull
  public String getBookmarkAddress()
  {
    return mBookmarkInfo.getAddress();
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
