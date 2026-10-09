package app.organicmaps.sdk.bookmarks.data;

import android.os.Parcel;
import androidx.annotation.NonNull;
import java.util.Objects;

public class OsmDescription implements MapObjectData
{
  @NonNull
  private final String mDescription;

  private OsmDescription(@NonNull String description)
  {
    mDescription = description;
  }

  @NonNull
  public String getDescription()
  {
    return mDescription;
  }

  @Override
  public int describeContents()
  {
    return 0;
  }

  @Override
  public void writeToParcel(@NonNull Parcel dest, int flags)
  {
    dest.writeString(mDescription);
  }

  public static final Creator<OsmDescription> CREATOR = new Creator<>() {
    @Override
    @NonNull
    public OsmDescription createFromParcel(@NonNull Parcel in)
    {
      return new OsmDescription(Objects.requireNonNull(in.readString(), "OsmDescription description is null"));
    }

    @Override
    @NonNull
    public OsmDescription[] newArray(int size)
    {
      return new OsmDescription[size];
    }
  };
}
