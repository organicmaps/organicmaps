package app.organicmaps.sdk.bookmarks.data;

import android.os.Parcel;
import android.os.Parcelable;
import androidx.annotation.ColorInt;
import androidx.annotation.DrawableRes;
import androidx.annotation.NonNull;
import app.organicmaps.BuildConfig;
import app.organicmaps.sdk.util.StringUtils;
import app.organicmaps.sdk.util.log.Logger;
import com.google.common.base.Objects;
import dalvik.annotation.optimization.FastNative;

public class Icon implements Parcelable
{
  private static final String TAG = Icon.class.getSimpleName();

  static final int BOOKMARK_ICON_TYPE_NONE = 0;

  @DrawableRes
  private static final int[] TYPE_ICONS = GetTypeIcons();

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

  @NonNull
  @DrawableRes
  private static int[] GetTypeIcons()
  {
    final String[] names = nativeGetBookmarkIconNames();
    int[] icons = new int[names.length];
    for (int i = 0; i < names.length; i++)
    {
      final String name = StringUtils.toSnakeCase(names[i]);
      try
      {
        icons[i] = app.organicmaps.sdk.R.drawable.class.getField("ic_bookmark_" + name).getInt(null);
      }
      catch (NoSuchFieldException | IllegalAccessException e)
      {
        Logger.e(TAG, "Error getting icon for " + name);
        // Force devs to add an icon for each bookmark type.
        if (BuildConfig.DEBUG)
          throw new RuntimeException("Error getting icon for " + name, e);
        icons[i] = app.organicmaps.sdk.R.drawable.ic_bookmark_none; // Fallback icon
      }
    }

    return icons;
  }

  @FastNative
  @NonNull
  private static native String[] nativeGetBookmarkIconNames();
}
