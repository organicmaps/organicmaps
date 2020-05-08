package com.mapswithme.maps.guides;

import android.os.Parcel;
import android.os.Parcelable;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.mapswithme.maps.gallery.Constants;
import com.mapswithme.maps.gallery.RegularAdapterStrategy;
import com.mapswithme.maps.widget.placepage.PlacePageData;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

public class GuidesGallery implements PlacePageData
{
  @NonNull
  private final List<Item> mItems;

  public GuidesGallery(@NonNull Item[] items)
  {
    mItems = Arrays.asList(items);
  }

  @NonNull
  public List<Item> getItems()
  {
    return Collections.unmodifiableList(mItems);
  }

  protected GuidesGallery(Parcel in)
  {
    List<Item> items = new ArrayList<>();
    in.readTypedList(items, Item.CREATOR);
    mItems = items;
  }

  @Override
  public int describeContents()
  {
    return 0;
  }

  @Override
  public void writeToParcel(Parcel dest, int flags)
  {
    // All collections are deserialized AFTER non-collection and primitive type objects,
    // so collections must be always serialized at the end.
    dest.writeTypedList(mItems);
  }

  public static final Creator<GuidesGallery> CREATOR = new Creator<GuidesGallery>()
  {
    @Override
    public GuidesGallery createFromParcel(Parcel in)
    {
      return new GuidesGallery(in);
    }

    @Override
    public GuidesGallery[] newArray(int size)
    {
      return new GuidesGallery[size];
    }
  };

  public static class Item extends RegularAdapterStrategy.Item
  {
    @NonNull
    private final String mGuideId;
    @NonNull
    private final String mImageUrl;
    @NonNull
    private final Type mType;
    private final boolean mDownloaded;
    @Nullable
    private final CityParams mCityParams;
    @Nullable
    private final OutdoorParams mOutdoorParams;
    private boolean mActivated = false;

    public Item(@NonNull String guideId, @NonNull String url, @NonNull String imageUrl,
                @NonNull String title, int type,
                boolean downloaded, @Nullable CityParams cityParams,
                @Nullable OutdoorParams outdoorParams)
    {
      super(Constants.TYPE_PRODUCT, title, "", url);
      mGuideId = guideId;
      mImageUrl = imageUrl;
      mType = Type.values()[type];
      mDownloaded = downloaded;
      mCityParams = cityParams;
      mOutdoorParams = outdoorParams;
    }

    protected Item(Parcel in)
    {
      super(in);
      mGuideId = in.readString();
      mImageUrl = in.readString();
      mType = Type.values()[in.readInt()];
      mDownloaded = in.readByte() != 0;
      mCityParams = in.readParcelable(CityParams.class.getClassLoader());
      mOutdoorParams = in.readParcelable(OutdoorParams.class.getClassLoader());
    }

    public static final Creator<Item> CREATOR = new Creator<Item>()
    {
      @Override
      public Item createFromParcel(Parcel in)
      {
        return new Item(in);
      }

      @Override
      public Item[] newArray(int size)
      {
        return new Item[size];
      }
    };

    @NonNull
    public String getGuideId()
    {
      return mGuideId;
    }


    @NonNull
    public String getImageUrl()
    {
      return mImageUrl;
    }

    @NonNull
    public Type getGuideType()
    {
      return mType;
    }

    public boolean isDownloaded()
    {
      return mDownloaded;
    }

    @Nullable
    public CityParams getCityParams()
    {
      return mCityParams;
    }

    @Nullable
    public OutdoorParams getOutdoorParams()
    {
      return mOutdoorParams;
    }

    public void setActivated(boolean activated)
    {
      mActivated = activated;
    }

    public boolean isActivated()
    {
      return mActivated;
    }

    @Override
    public int describeContents()
    {
      return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags)
    {
      super.writeToParcel(dest, flags);
      dest.writeString(mGuideId);
      dest.writeString(mImageUrl);
      dest.writeInt(mType.ordinal());
      dest.writeByte((byte) (mDownloaded ? 1 : 0));
      dest.writeParcelable(mCityParams, flags);
      dest.writeParcelable(mOutdoorParams, flags);
    }
  }

  public static class CityParams implements Parcelable
  {
    private final int mBookmarksCount;
    private final boolean mIsTrackAvailable;

    public CityParams(int bookmarksCount, boolean isTrackAvailable)
    {
      mBookmarksCount = bookmarksCount;
      mIsTrackAvailable = isTrackAvailable;
    }

    protected CityParams(Parcel in)
    {
      mBookmarksCount = in.readInt();
      mIsTrackAvailable = in.readByte() != 0;
    }

    public static final Creator<CityParams> CREATOR = new Creator<CityParams>()
    {
      @Override
      public CityParams createFromParcel(Parcel in)
      {
        return new CityParams(in);
      }

      @Override
      public CityParams[] newArray(int size)
      {
        return new CityParams[size];
      }
    };

    public int getBookmarksCount()
    {
      return mBookmarksCount;
    }

    public boolean isTrackAvailable()
    {
      return mIsTrackAvailable;
    }

    @Override
    public int describeContents()
    {
      return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags)
    {
      dest.writeInt(mBookmarksCount);
      dest.writeByte((byte) (mIsTrackAvailable ? 1 : 0));
    }
  }

  public static class OutdoorParams implements Parcelable
  {
    @NonNull
    private final String mTag;
    private final double mDistance;
    private final long mDuration;
    private final int mAscent;

    public OutdoorParams(@NonNull String tag, double distance, long duration, int ascent)
    {
      mTag = tag;
      mDistance = distance;
      mDuration = duration;
      mAscent = ascent;
    }

    protected OutdoorParams(Parcel in)
    {
      mTag = in.readString();
      mDistance = in.readDouble();
      mDuration = in.readLong();
      mAscent = in.readInt();
    }

    public static final Creator<OutdoorParams> CREATOR = new Creator<OutdoorParams>()
    {
      @Override
      public OutdoorParams createFromParcel(Parcel in)
      {
        return new OutdoorParams(in);
      }

      @Override
      public OutdoorParams[] newArray(int size)
      {
        return new OutdoorParams[size];
      }
    };

    @NonNull
    public String getString()
    {
      return mTag;
    }

    public double getDistance()
    {
      return mDistance;
    }

    public long getDuration()
    {
      return mDuration;
    }

    public int getAscent()
    {
      return mAscent;
    }

    @Override
    public int describeContents()
    {
      return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags)
    {
      dest.writeString(mTag);
      dest.writeDouble(mDistance);
      dest.writeLong(mDuration);
      dest.writeInt(mAscent);
    }
  }

  public enum Type
  {
    City,
    Outdoor
  }
}
