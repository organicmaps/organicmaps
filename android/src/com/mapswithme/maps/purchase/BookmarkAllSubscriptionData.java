package com.mapswithme.maps.purchase;

import android.os.Parcel;
import android.os.Parcelable;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

public class BookmarkAllSubscriptionData implements Parcelable
{
  @SuppressWarnings("unused")
  public static final Parcelable.Creator<BookmarkAllSubscriptionData> CREATOR =
      new Parcelable.Creator<BookmarkAllSubscriptionData>()
  {
    @Override
    public BookmarkAllSubscriptionData createFromParcel(Parcel in)
    {
      return new BookmarkAllSubscriptionData(in);
    }

    @Override
    public BookmarkAllSubscriptionData[] newArray(int size)
    {
      return new BookmarkAllSubscriptionData[size];
    }
  };
  @Nullable
  private final List<BookmarksAllSubscriptionPage> mOrderList;

  public BookmarkAllSubscriptionData(@NonNull List<BookmarksAllSubscriptionPage> orderList)
  {
    mOrderList = Collections.unmodifiableList(orderList);
  }

  public BookmarkAllSubscriptionData(@NonNull BookmarksAllSubscriptionPage... arguments)
  {
    this(Arrays.asList(arguments));
  }

  protected BookmarkAllSubscriptionData(Parcel in)
  {
    if (in.readByte() == 0x01)
    {
      List<Integer> indexList = new ArrayList<>();
      in.readList(indexList, Integer.class.getClassLoader());
      mOrderList = new ArrayList<>(indexList.size());
      for (Integer index : indexList)
      {
        mOrderList.add(BookmarksAllSubscriptionPage.values()[index]);
      }
    }
    else
    {
      mOrderList = null;
    }
  }

  @NonNull
  public List<BookmarksAllSubscriptionPage> getOrderList()
  {

    return mOrderList != null ? mOrderList : Collections.emptyList();
  }

  @Override
  public int describeContents()
  {
    return 0;
  }

  @Override
  public void writeToParcel(Parcel dest, int flags)
  {
    if (mOrderList == null)
    {
      dest.writeByte((byte) (0x00));
    }
    else
    {
      dest.writeByte((byte) (0x01));
      List<Integer> indexArray = new ArrayList<>(mOrderList.size());
      for (BookmarksAllSubscriptionPage page : mOrderList)
      {
        indexArray.add(page.ordinal());
      }
      dest.writeList(indexArray);
    }
  }
}
