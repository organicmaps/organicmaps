package app.organicmaps.sdk.bookmarks.data;

import androidx.annotation.IntDef;
import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.Objects;

@Keep
@SuppressWarnings("unused")
public final class BookmarkListRow
{
  @Retention(RetentionPolicy.SOURCE)
  @IntDef({Type.TRACK, Type.BOOKMARK, Type.SECTION, Type.DESCRIPTION})
  public @interface Type
  {
    int TRACK = 0;
    int BOOKMARK = 1;
    int SECTION = 2;
    int DESCRIPTION = 3;
  }

  @Retention(RetentionPolicy.SOURCE)
  @IntDef({SectionKind.NONE, SectionKind.DESCRIPTION, SectionKind.TRACKS, SectionKind.BOOKMARKS, SectionKind.CUSTOM})
  public @interface SectionKind
  {
    int NONE = 0;
    int DESCRIPTION = 1;
    int TRACKS = 2;
    int BOOKMARKS = 3;
    int CUSTOM = 4;
  }

  @Type
  private final int mType;
  private final long mStableId;
  @SectionKind
  private final int mSectionKind;
  @Nullable
  private final String mTitle;
  @Nullable
  private final String mDescription;
  @Nullable
  private final BookmarkInfo mBookmark;
  @Nullable
  private final Track mTrack;

  @Keep
  private BookmarkListRow(@Type int type, long stableId, @SectionKind int sectionKind, @Nullable String title,
                          @Nullable String description, @Nullable BookmarkInfo bookmark, @Nullable Track track)
  {
    mType = type;
    mStableId = stableId;
    mSectionKind = sectionKind;
    mTitle = title;
    mDescription = description;
    mBookmark = bookmark;
    mTrack = track;
  }

  @NonNull
  public static BookmarkListRow section(long stableId, @SectionKind int sectionKind, @Nullable String title)
  {
    return new BookmarkListRow(Type.SECTION, stableId, sectionKind, title, null, null, null);
  }

  @NonNull
  public static BookmarkListRow description(long stableId, @NonNull String title, @NonNull String description)
  {
    return new BookmarkListRow(Type.DESCRIPTION, stableId, SectionKind.NONE, title, description, null, null);
  }

  @NonNull
  public static BookmarkListRow bookmark(@NonNull BookmarkInfo bookmark)
  {
    return new BookmarkListRow(Type.BOOKMARK, bookmark.getBookmarkId(), SectionKind.NONE, null, null, bookmark, null);
  }

  @NonNull
  public static BookmarkListRow track(@NonNull Track track)
  {
    return new BookmarkListRow(Type.TRACK, -track.getTrackId() - 1, SectionKind.NONE, null, null, null, track);
  }

  @Type
  public int getType()
  {
    return mType;
  }

  public long getStableId()
  {
    return mStableId;
  }

  @SectionKind
  public int getSectionKind()
  {
    return mSectionKind;
  }

  @Nullable
  public String getTitle()
  {
    return mTitle;
  }

  @Nullable
  public String getDescription()
  {
    return mDescription;
  }

  @Nullable
  public BookmarkInfo getBookmark()
  {
    return mBookmark;
  }

  @Nullable
  public Track getTrack()
  {
    return mTrack;
  }

  @Override
  public boolean equals(Object o)
  {
    if (this == o)
      return true;
    if (o == null || getClass() != o.getClass())
      return false;
    BookmarkListRow that = (BookmarkListRow) o;
    return mType == that.mType && mStableId == that.mStableId && mSectionKind == that.mSectionKind
 && Objects.equals(mTitle, that.mTitle) && Objects.equals(mDescription, that.mDescription)
 && Objects.equals(mBookmark, that.mBookmark) && Objects.equals(mTrack, that.mTrack);
  }

  @Override
  public int hashCode()
  {
    return Objects.hash(mType, mStableId, mSectionKind, mTitle, mDescription, mBookmark, mTrack);
  }
}
