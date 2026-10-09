package app.organicmaps.sdk.bookmarks.data;

import android.os.Parcel;
import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import java.util.Objects;

public class ApiData implements MapObjectData
{
  @NonNull
  private final String mId;
  @NonNull
  private final String mUrl;

  @Keep
  private ApiData(@NonNull String id, @NonNull String url)
  {
    mId = id;
    mUrl = url;
  }

  @NonNull
  public String getId()
  {
    return mId;
  }

  @NonNull
  public String getUrl()
  {
    return mUrl;
  }

  @Override
  public int describeContents()
  {
    return 0;
  }

  @Override
  public void writeToParcel(@NonNull Parcel dest, int flags)
  {
    dest.writeString(mId);
    dest.writeString(mUrl);
  }

  public static final Creator<ApiData> CREATOR = new Creator<>() {
    @Override
    @NonNull
    public ApiData createFromParcel(@NonNull Parcel source)
    {
      return new ApiData(Objects.requireNonNull(source.readString(), "id must not be null"),
                         Objects.requireNonNull(source.readString(), "url must not be null"));
    }

    @Override
    @NonNull
    public ApiData[] newArray(int size)
    {
      return new ApiData[size];
    }
  };
}
