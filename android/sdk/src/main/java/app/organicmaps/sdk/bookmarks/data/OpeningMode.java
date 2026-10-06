package app.organicmaps.sdk.bookmarks.data;

import android.os.Parcel;
import androidx.annotation.NonNull;

// TODO: Remove this class. The opening mode should be decided in the app layer, not in the core layer.
public enum OpeningMode implements MapObjectData
{
  PREVIEW(0),
  PREVIEW_PLUS(1),
  DETAILS(2),
  FULL(3);

  private final int mValue;

  OpeningMode(int value)
  {
    mValue = value;
  }

  @NonNull
  public static OpeningMode fromInt(int value)
  {
    for (final OpeningMode mode : values())
    {
      if (mode.mValue == value)
        return mode;
    }
    throw new IllegalArgumentException("Invalid OpeningMode value: " + value);
  }

  @Override
  public int describeContents()
  {
    return 0;
  }

  @Override
  public void writeToParcel(@NonNull Parcel dest, int flags)
  {
    dest.writeInt(mValue);
  }

  public static final Creator<OpeningMode> CREATOR = new Creator<>() {
    @Override
    @NonNull
    public OpeningMode createFromParcel(@NonNull Parcel source)
    {
      return fromInt(source.readInt());
    }

    @Override
    @NonNull
    public OpeningMode[] newArray(int size)
    {
      return new OpeningMode[size];
    }
  };
}
