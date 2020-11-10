package com.mapswithme.maps.purchase;

import android.os.Parcel;
import android.os.Parcelable;

import androidx.annotation.DrawableRes;
import androidx.annotation.NonNull;

public class BookmarksAllSubscriptionPageData implements Parcelable
{
  @SuppressWarnings("unused")
  public static final Parcelable.Creator<BookmarksAllSubscriptionPageData> CREATOR =
      new Parcelable.Creator<BookmarksAllSubscriptionPageData>()
  {
    @Override
    public BookmarksAllSubscriptionPageData createFromParcel(Parcel in)
    {
      return new BookmarksAllSubscriptionPageData(in);
    }

    @Override
    public BookmarksAllSubscriptionPageData[] newArray(int size)
    {
      return new BookmarksAllSubscriptionPageData[size];
    }
  };

  @DrawableRes
  private final int mImageId;
  @NonNull
  private final BookmarksAllSubscriptionPage mPage;

  public BookmarksAllSubscriptionPageData(@DrawableRes int imageId, @NonNull BookmarksAllSubscriptionPage page)
  {
    mImageId = imageId;
    mPage = page;
  }

  protected BookmarksAllSubscriptionPageData(Parcel in)
  {
    mImageId = in.readInt();
    mPage = BookmarksAllSubscriptionPage.values()[in.readInt()];
  }

  @DrawableRes
  public int getImageId()
  {
    return mImageId;
  }

  @NonNull
  public BookmarksAllSubscriptionPage getPage()
  {
    return mPage;
  }

  @Override
  public int describeContents()
  {
    return 0;
  }

  @Override
  public void writeToParcel(Parcel dest, int flags)
  {
    dest.writeInt(mImageId);
    dest.writeInt(mPage.ordinal());
  }
}
