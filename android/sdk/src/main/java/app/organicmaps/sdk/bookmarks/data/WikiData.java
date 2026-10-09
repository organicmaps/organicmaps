package app.organicmaps.sdk.bookmarks.data;

import android.os.Parcel;
import androidx.annotation.NonNull;
import java.util.Objects;

public class WikiData implements MapObjectData
{
  @NonNull
  private final String mArticle;

  private WikiData(@NonNull String article)
  {
    mArticle = article;
  }

  @NonNull
  public String getArticle()
  {
    return mArticle;
  }

  @Override
  public int describeContents()
  {
    return 0;
  }

  @Override
  public void writeToParcel(@NonNull Parcel dest, int flags)
  {
    dest.writeString(mArticle);
  }

  public static final Creator<WikiData> CREATOR = new Creator<>() {
    @Override
    @NonNull
    public WikiData createFromParcel(@NonNull Parcel source)
    {
      return new WikiData(Objects.requireNonNull(source.readString(), "article must not be null"));
    }

    @Override
    @NonNull
    public WikiData[] newArray(int size)
    {
      return new WikiData[size];
    }
  };
}
