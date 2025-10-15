package app.organicmaps.sdk.bookmarks.data;

import android.os.Parcel;
import android.os.Parcelable;
import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import java.util.Objects;

// Used by JNI.
@Keep
@SuppressWarnings("unused")
public class BookmarkCategory implements Parcelable
{
  private final long mId;
  @NonNull
  private final String mName;
  @NonNull
  private final String mAnnotation;
  @NonNull
  private final String mDescription;
  private final int mTracksCount;
  private final int mBookmarksCount;
  private boolean mIsVisible;

  public BookmarkCategory(long id, @NonNull String name, @NonNull String annotation, @NonNull String description,
                          int tracksCount, int bookmarksCount, boolean isVisible)
  {
    mId = id;
    mName = name;
    mAnnotation = annotation;
    mDescription = description;
    mTracksCount = tracksCount;
    mBookmarksCount = bookmarksCount;
    mIsVisible = isVisible;
  }

  @Override
  public boolean equals(Object o)
  {
    if (this == o)
      return true;
    if (o == null || getClass() != o.getClass())
      return false;
    BookmarkCategory that = (BookmarkCategory) o;
    return mId == that.mId;
  }

  @Override
  public int hashCode()
  {
    return Long.hashCode(mId);
  }

  public long getId()
  {
    return mId;
  }

  @NonNull
  public String getName()
  {
    return mName;
  }

  public int getTracksCount()
  {
    return mTracksCount;
  }

  public int getBookmarksCount()
  {
    return mBookmarksCount;
  }

  public boolean isVisible()
  {
    return mIsVisible;
  }

  public void setVisible(boolean isVisible)
  {
    mIsVisible = isVisible;
  }

  public int size()
  {
    return getBookmarksCount() + getTracksCount();
  }

  @NonNull
  public String getAnnotation()
  {
    return mAnnotation;
  }

  @NonNull
  public String getDescription()
  {
    return mDescription;
  }

  @Override
  @NonNull
  public String toString()
  {
    final StringBuilder sb = new StringBuilder("BookmarkCategory{");
    sb.append("mId=").append(mId);
    sb.append(", mName='").append(mName).append('\'');
    sb.append(", mAnnotation='").append(mAnnotation).append('\'');
    sb.append(", mDescription='").append(mDescription).append('\'');
    sb.append(", mTracksCount=").append(mTracksCount);
    sb.append(", mBookmarksCount=").append(mBookmarksCount);
    sb.append(", mIsVisible=").append(mIsVisible);

    sb.append('}');
    return sb.toString();
  }

  @Override
  public int describeContents()
  {
    return 0;
  }

  @Override
  public void writeToParcel(@NonNull Parcel dest, int flags)
  {
    dest.writeLong(this.mId);
    dest.writeString(this.mName);
    dest.writeString(this.mAnnotation);
    dest.writeString(this.mDescription);
    dest.writeInt(this.mTracksCount);
    dest.writeInt(this.mBookmarksCount);
    dest.writeByte(this.mIsVisible ? (byte) 1 : (byte) 0);
  }

  private BookmarkCategory(@NonNull Parcel in)
  {
    this.mId = in.readLong();
    this.mName = Objects.requireNonNull(in.readString());
    this.mAnnotation = Objects.requireNonNull(in.readString());
    this.mDescription = Objects.requireNonNull(in.readString());
    this.mTracksCount = in.readInt();
    this.mBookmarksCount = in.readInt();
    this.mIsVisible = in.readByte() != 0;
  }

  public static final Creator<BookmarkCategory> CREATOR = new Creator<>() {
    @Override
    public BookmarkCategory createFromParcel(Parcel source)
    {
      return new BookmarkCategory(source);
    }

    @Override
    public BookmarkCategory[] newArray(int size)
    {
      return new BookmarkCategory[size];
    }
  };
}
