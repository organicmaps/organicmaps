package app.organicmaps.sdk.bookmarks.data;

import android.os.Parcel;
import android.os.Parcelable;
import androidx.annotation.IntDef;
import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.Objects;

public final class BookmarkCategory implements Parcelable
{
  @Retention(RetentionPolicy.SOURCE)
  @IntDef({SortingType.BY_TYPE, SortingType.BY_DISTANCE, SortingType.BY_TIME, SortingType.BY_NAME})
  public @interface SortingType
  {
    int BY_TYPE = 0;
    int BY_DISTANCE = 1;
    int BY_TIME = 2;
    int BY_NAME = 3;
  }

  private final long mId;
  @NonNull
  private String mName;
  @NonNull
  private final String mAnnotation;
  @NonNull
  private String mDescription;
  private final int mTracksCount;
  private final int mBookmarksCount;
  private boolean mIsVisible;

  // Used by JNI.
  @Keep
  @SuppressWarnings("unused")
  private BookmarkCategory(long id, @NonNull String name, @NonNull String annotation, @NonNull String description,
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

  public void setName(@NonNull String name)
  {
    mName = name;
    nativeSetName(mId, mName);
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
    mIsVisible = nativeIsVisible(mId);
    return mIsVisible;
  }

  public void setVisibility(boolean isVisible)
  {
    mIsVisible = isVisible;
    nativeSetVisibility(mId, mIsVisible);
  }

  public void toggleVisibility()
  {
    setVisibility(!isVisible());
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

  public void setDescription(@NonNull String description)
  {
    mDescription = description;
    nativeSetDescription(mId, mDescription);
  }

  public long getBookmarkIdByPosition(int positionInCategory)
  {
    return nativeGetBookmarkIdByPosition(mId, positionInCategory);
  }

  public long getTrackIdByPosition(int positionInCategory)
  {
    return nativeGetTrackIdByPosition(mId, positionInCategory);
  }

  public boolean hasLastSortingType()
  {
    return nativeHasLastSortingType(mId);
  }

  @SortingType
  public int getLastSortingType()
  {
    return nativeGetLastSortingType(mId);
  }

  public void setLastSortingType(@SortingType int sortingType)
  {
    nativeSetLastSortingType(mId, sortingType);
  }

  public void resetLastSortingType()
  {
    nativeResetLastSortingType(mId);
  }

  @NonNull
  @SortingType
  public int[] getAvailableSortingTypes(boolean hasMyPosition)
  {
    return nativeGetAvailableSortingTypes(mId, hasMyPosition);
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
    @NonNull
    @Override
    public BookmarkCategory createFromParcel(@NonNull Parcel source)
    {
      return new BookmarkCategory(source);
    }

    @Override
    public BookmarkCategory[] newArray(int size)
    {
      return new BookmarkCategory[size];
    }
  };

  private static native void nativeSetName(long catId, @NonNull String name);
  private static native void nativeSetDescription(long catId, @NonNull String desc);
  private static native boolean nativeIsVisible(long catId);
  private static native void nativeSetVisibility(long catId, boolean visible);
  private static native void nativeSetTags(long catId, @NonNull String[] tagsIds);
  private static native void nativeSetAccessRules(long catId, int accessRules);
  private static native void nativeSetCustomProperty(long catId, String key, String value);
  private static native boolean nativeIsEmpty(long catId);

  private static native long nativeGetBookmarkIdByPosition(long catId, int position);
  private static native long nativeGetTrackIdByPosition(long catId, int position);

  private static native boolean nativeHasLastSortingType(long catId);
  @SortingType
  private static native int nativeGetLastSortingType(long catId);
  private static native void nativeSetLastSortingType(long catId, @SortingType int sortingType);
  private static native void nativeResetLastSortingType(long catId);
  @NonNull
  @SortingType
  private static native int[] nativeGetAvailableSortingTypes(long catId, boolean hasMyPosition);
}
