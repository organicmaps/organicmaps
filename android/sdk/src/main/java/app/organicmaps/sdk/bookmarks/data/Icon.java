package app.organicmaps.sdk.bookmarks.data;

import android.content.res.Resources;
import android.os.Parcel;
import android.os.Parcelable;
import androidx.annotation.ColorInt;
import androidx.annotation.DrawableRes;
import androidx.annotation.NonNull;
import app.organicmaps.sdk.BuildConfig;
import app.organicmaps.sdk.R;
import app.organicmaps.sdk.util.StringUtils;
import app.organicmaps.sdk.util.log.Logger;
import dalvik.annotation.optimization.FastNative;
import java.util.Arrays;

public class Icon implements Parcelable
{
  private static final String TAG = Icon.class.getSimpleName();

  @DrawableRes
  private static int[] sTypeIcons = null;

  @PredefinedColors.Color
  private final int mColor;
  private final int mType;

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
    // loadDefaultIcons should be called
    assert (sTypeIcons != null);
    return sTypeIcons[mType];
  }

  public int getType()
  {
    return mType;
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
    return Arrays.hashCode(new int[] {mColor, mType});
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

  static public void loadDefaultIcons(@NonNull Resources resources, @NonNull String packageName)
  {
    final String[] names = nativeGetBookmarkIconNames();
    int[] icons = new int[names.length];
    for (int i = 0; i < names.length; i++)
    {
      final String name = StringUtils.toSnakeCase(names[i]);
      icons[i] = resources.getIdentifier("ic_bookmark_" + name, "drawable", packageName);
      if (icons[i] == 0)
      {
        Logger.e(TAG, "Error getting icon for " + name);
        // Force devs to add an icon for each bookmark type.
        if (BuildConfig.DEBUG)
          throw new RuntimeException("Error getting icon for " + name);
        icons[i] = R.drawable.ic_bookmark_none; // Fallback icon
      }
    }

    sTypeIcons = icons;
  }

  @FastNative
  @NonNull
  private static native String[] nativeGetBookmarkIconNames();
}
