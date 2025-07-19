package app.organicmaps.sdk.bookmarks.data;

import android.os.Parcel;
import android.os.Parcelable;
import androidx.annotation.ColorInt;
import androidx.annotation.DrawableRes;
import app.organicmaps.sdk.R;
import com.google.common.base.Objects;

public class Icon implements Parcelable
{
  static final int BOOKMARK_ICON_TYPE_NONE = 0;

  /// @note Important! Should be synced with kml/types.hpp/BookmarkIcon
  /// @todo Can make better: take name-by-type from Core and make a concat: "R.drawable.ic_bookmark_" + name.
  // First icon should be "none" <-> BOOKMARK_ICON_TYPE_NONE.
  @DrawableRes
  private static final int[] TYPE_ICONS = {
      R.drawable.ic_bookmark_none,          R.drawable.ic_bookmark_hotel,     R.drawable.ic_bookmark_animals,
      R.drawable.ic_bookmark_buddhism,      R.drawable.ic_bookmark_building,  R.drawable.ic_bookmark_christianity,
      R.drawable.ic_bookmark_entertainment, R.drawable.ic_bookmark_money,     R.drawable.ic_bookmark_food,
      R.drawable.ic_bookmark_gas,           R.drawable.ic_bookmark_judaism,   R.drawable.ic_bookmark_medicine,
      R.drawable.ic_bookmark_mountain,      R.drawable.ic_bookmark_museum,    R.drawable.ic_bookmark_islam,
      R.drawable.ic_bookmark_park,          R.drawable.ic_bookmark_parking,   R.drawable.ic_bookmark_shop,
      R.drawable.ic_bookmark_sights,        R.drawable.ic_bookmark_swim,      R.drawable.ic_bookmark_water,
      R.drawable.ic_bookmark_bar,           R.drawable.ic_bookmark_transport, R.drawable.ic_bookmark_viewpoint,
      R.drawable.ic_bookmark_sport,
      R.drawable.ic_bookmark_none, // pub
      R.drawable.ic_bookmark_none, // art
      R.drawable.ic_bookmark_none, // bank
      R.drawable.ic_bookmark_none, // cafe
      R.drawable.ic_bookmark_none, // pharmacy
      R.drawable.ic_bookmark_none, // stadium
      R.drawable.ic_bookmark_none, // theatre
      R.drawable.ic_bookmark_none, // information
      R.drawable.ic_bookmark_none, // ChargingStation
      R.drawable.ic_bookmark_none, // BicycleParking
      R.drawable.ic_bookmark_none, // BicycleParkingCovered
      R.drawable.ic_bookmark_none, // BicycleRental
      R.drawable.ic_bookmark_none  // FastFood
  };

  @PredefinedColors.Color
  private final int mColor;
  private final int mType;

  public Icon(@PredefinedColors.Color int color)
  {
    this(color, BOOKMARK_ICON_TYPE_NONE);
  }

  public Icon(@PredefinedColors.Color int color, int type)
  {
    mColor = color;
    mType = type;
  }

  @Override
  public int describeContents()
  {
    return 0;
  }

  @Override
  public void writeToParcel(Parcel dest, int flags)
  {
    dest.writeInt(mColor);
    dest.writeInt(mType);
  }

  private Icon(Parcel in)
  {
    mColor = in.readInt();
    mType = in.readInt();
  }

  @PredefinedColors.Color
  public int getColor()
  {
    return mColor;
  }

  @ColorInt
  public int argb()
  {
    return PredefinedColors.getColor(mColor);
  }

  @DrawableRes
  public int getResId()
  {
    return TYPE_ICONS[mType];
  }

  @Override
  public boolean equals(Object o)
  {
    if (this == o)
      return true;
    if (o instanceof Icon comparedIcon)
      return mColor == comparedIcon.mColor && mType == comparedIcon.mType;
    return false;
  }

  @Override
  public int hashCode()
  {
    return Objects.hashCode(mColor, mType);
  }

  public static final Parcelable.Creator<Icon> CREATOR = new Parcelable.Creator<>() {
    public Icon createFromParcel(Parcel in)
    {
      return new Icon(in);
    }

    public Icon[] newArray(int size)
    {
      return new Icon[size];
    }
  };
}
