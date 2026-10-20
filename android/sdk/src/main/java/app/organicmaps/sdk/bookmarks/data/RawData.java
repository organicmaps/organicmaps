package app.organicmaps.sdk.bookmarks.data;

import android.os.Parcel;
import androidx.annotation.NonNull;
import java.util.Collections;
import java.util.HashSet;
import java.util.Objects;
import java.util.Set;

public class RawData implements MapObjectData
{
  @NonNull
  private final Set<String> mRawData;

  private RawData(@NonNull String[] data)
  {
    mRawData = new HashSet<>();
    Collections.addAll(mRawData, data);
  }

  public boolean has(@NonNull String type)
  {
    return mRawData.contains(type);
  }

  @Override
  public int describeContents()
  {
    return 0;
  }

  @Override
  public void writeToParcel(@NonNull Parcel dest, int flags)
  {
    dest.writeStringArray(mRawData.toArray(new String[0]));
  }

  public static final Creator<RawData> CREATOR = new Creator<>() {
    @Override
    @NonNull
    public RawData createFromParcel(@NonNull Parcel source)
    {
      return new RawData(Objects.requireNonNull(source.createStringArray(), "raw data must not be null"));
    }

    @Override
    @NonNull
    public RawData[] newArray(int size)
    {
      return new RawData[size];
    }
  };
}
