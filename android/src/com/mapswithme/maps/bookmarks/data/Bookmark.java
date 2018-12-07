package com.mapswithme.maps.bookmarks.data;

import android.annotation.SuppressLint;
import android.os.Parcel;
import android.support.annotation.IntRange;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.ads.Banner;
import com.mapswithme.maps.ads.LocalAdInfo;
import com.mapswithme.maps.routing.RoutePointInfo;
import com.mapswithme.maps.search.HotelsFilter;
import com.mapswithme.maps.search.Popularity;
import com.mapswithme.maps.search.PriceFilterView;
import com.mapswithme.maps.ugc.UGC;
import com.mapswithme.util.Constants;

// TODO consider refactoring to remove hack with MapObject unmarshalling itself and Bookmark at the same time.
@SuppressLint("ParcelCreator")
public class Bookmark extends MapObject
{
  private final Icon mIcon;
  private long mCategoryId;
  private long mBookmarkId;
  private double mMerX;
  private double mMerY;

  public Bookmark(@NonNull FeatureId featureId, @IntRange(from = 0) long categoryId,
                  @IntRange(from = 0) long bookmarkId, String title, @Nullable String secondaryTitle,
                  @Nullable String subtitle, @Nullable String address, @Nullable Banner[] banners,
                  @Nullable int[] reachableByTaxiTypes, @Nullable String bookingSearchUrl,
                  @Nullable LocalAdInfo localAdInfo, @Nullable RoutePointInfo routePointInfo,
                  boolean isExtendedView, boolean shouldShowUGC, boolean canBeRated,
                  boolean canBeReviewed, @Nullable UGC.Rating[] ratings,
                  @Nullable HotelsFilter.HotelType hotelType, @PriceFilterView.PriceDef int priceRate,
                  @NonNull Popularity popularity, @NonNull String description)
  {
    super(featureId, BOOKMARK, title, secondaryTitle, subtitle, address, 0, 0, "",
          banners, reachableByTaxiTypes, bookingSearchUrl, localAdInfo, routePointInfo,
          isExtendedView, shouldShowUGC, canBeRated, canBeReviewed, ratings, hotelType, priceRate,
          popularity, description);

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
    dest.writeInt(mIcon.getColor());
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
    mIcon = BookmarkManager.INSTANCE.getIconByColor(source.readInt());
    mMerX = source.readDouble();
    mMerY = source.readDouble();
    initXY();
  }

  @Override
  public double getScale()
  {
    return nativeGetScale(mBookmarkId);
  }

  public DistanceAndAzimut getDistanceAndAzimuth(double cLat, double cLon, double north)
  {
    return Framework.nativeGetDistanceAndAzimuth(mMerX, mMerY, cLat, cLon, north);
  }

  private Icon getIconInternal()
  {
    return BookmarkManager.INSTANCE.getIconByColor(nativeGetColor(mBookmarkId));
  }

  public Icon getIcon()
  {
    return mIcon;
  }

  @Override
  @MapObjectType
  public int getMapObjectType()
  {
    return MapObject.BOOKMARK;
  }

  public String getCategoryName()
  {
    return BookmarkManager.INSTANCE.getCategoryName(mCategoryId);
  }

  public void setCategoryId(@IntRange(from = 0) long catId)
  {
    if (catId == mCategoryId)
      return;

    nativeChangeCategory(mCategoryId, catId, mBookmarkId);
    mCategoryId = catId;
  }

  public void setParams(String title, Icon icon, String description)
  {
    if (icon == null)
      icon = mIcon;

    if (!title.equals(getTitle()) || icon != mIcon || !description.equals(getBookmarkDescription()))
    {
      nativeSetBookmarkParams(mBookmarkId, title,
                              icon != null ? icon.getColor()
                                           : BookmarkManager.INSTANCE.getLastEditedColor(),
                              description);
    }
  }

  public long getCategoryId()
  {
    return mCategoryId;
  }

  public long getBookmarkId()
  {
    return mBookmarkId;
  }

  public String getBookmarkDescription()
  {
    return nativeGetBookmarkDescription(mBookmarkId);
  }

  public String getGe0Url(boolean addName)
  {
    return nativeEncode2Ge0Url(mBookmarkId, addName);
  }

  public String getHttpGe0Url(boolean addName)
  {
    return getGe0Url(addName).replaceFirst(Constants.Url.GE0_PREFIX, Constants.Url.HTTP_GE0_PREFIX);
  }

  public static native String nativeGetName(@IntRange(from = 0) long bookmarkId);

  public static native ParcelablePointD nativeGetXY(@IntRange(from = 0) long bookmarkId);

  public static native int nativeGetColor(@IntRange(from = 0) long bookmarkId);

  private native String nativeGetBookmarkDescription(@IntRange(from = 0) long bookmarkId);

  private native double nativeGetScale(@IntRange(from = 0) long bookmarkId);

  private native String nativeEncode2Ge0Url(@IntRange(from = 0) long bookmarkId, boolean addName);

  private native void nativeSetBookmarkParams(@IntRange(from = 0) long bookmarkId, String name, int color, String descr);

  private native void nativeChangeCategory(@IntRange(from = 0) long oldCatId, @IntRange(from = 0) long newCatId, @IntRange(from = 0) long bookmarkId);
}
