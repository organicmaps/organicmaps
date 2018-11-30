package com.mapswithme.maps.bookmarks.data;

import android.content.Context;
import android.content.res.Resources;
import android.os.Parcel;
import android.os.Parcelable;
import android.support.annotation.DrawableRes;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.annotation.PluralsRes;
import android.support.annotation.StringRes;
import android.text.TextUtils;

import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.BookmarksPageFactory;
import com.mapswithme.util.TypeConverter;
import com.mapswithme.util.UiUtils;

public class BookmarkCategory implements Parcelable
{
  private final long mId;
  @NonNull
  private final String mName;
  @Nullable
  private final Author mAuthor;
  @NonNull
  private final String mAnnotation;
  @NonNull
  private final String mDescription;
  private final int mTracksCount;
  private final int mBookmarksCount;
  private final int mTypeIndex;
  private final int mAccessRulesIndex;
  private final boolean mIsMyCategory;
  private final boolean mIsVisible;


  public BookmarkCategory(long id, @NonNull String name, @NonNull String authorId,
                          @NonNull String authorName, @NonNull String annotation,
                          @NonNull String description, int tracksCount, int bookmarksCount,
                          boolean fromCatalog, boolean isMyCategory, boolean isVisible,
                          int accessRulesIndex)
  {
    mId = id;
    mName = name;
    mAnnotation = annotation;
    mDescription = description;
    mTracksCount = tracksCount;
    mBookmarksCount = bookmarksCount;
    mIsMyCategory = isMyCategory;
    mTypeIndex = fromCatalog && !isMyCategory ? Type.DOWNLOADED.ordinal() : Type.PRIVATE.ordinal();
    mIsVisible = isVisible;
    mAuthor = TextUtils.isEmpty(authorId) || TextUtils.isEmpty(authorName)
              ? null
              : new Author(authorId, authorName);
    mAccessRulesIndex = accessRulesIndex;
  }

  @Override
  public boolean equals(Object o)
  {
    if (this == o) return true;
    if (o == null || getClass() != o.getClass()) return false;
    BookmarkCategory that = (BookmarkCategory) o;
    return mId == that.mId;
  }

  @Override
  public int hashCode()
  {
    return (int)(mId ^ (mId >>> 32));
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

  @Nullable
  public Author getAuthor()
  {
    return mAuthor;
  }

  public int getTracksCount()
  {
    return mTracksCount;
  }

  public int getBookmarksCount()
  {
    return mBookmarksCount;
  }

  @NonNull
  public Type getType()
  {
    return Type.values()[mTypeIndex];
  }

  @NonNull
  public AccessRules getAccessRules()
  {
    return AccessRules.values()[mAccessRulesIndex];
  }

  public boolean isFromCatalog()
  {
    return Type.values()[mTypeIndex] == Type.DOWNLOADED;
  }

  public boolean isVisible()
  {
    return mIsVisible;
  }

  public boolean isMyCategory()
  {
    return mIsMyCategory;
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

  @NonNull
  public CountAndPlurals getPluralsCountTemplate()
  {
    if (size() == 0)
      return new CountAndPlurals(0, R.plurals.objects);

    if (getBookmarksCount() == 0)
      return new CountAndPlurals(getTracksCount(), R.plurals.tracks);

    if (getTracksCount() == 0)
      return new CountAndPlurals(getBookmarksCount(), R.plurals.places);

    return new CountAndPlurals(size(), R.plurals.objects);
  }

  public boolean isSharingOptionsAllowed()
  {
    BookmarkCategory.AccessRules rules = getAccessRules();
    return rules != BookmarkCategory.AccessRules.ACCESS_RULES_PAID
           && rules != BookmarkCategory.AccessRules.ACCESS_RULES_P2P
           && size() > 0;
  }

  public boolean isExportAllowed()
  {
    BookmarkCategory.AccessRules rules = getAccessRules();
    boolean isLocal = rules == BookmarkCategory.AccessRules.ACCESS_RULES_LOCAL;
    return isLocal && size() > 0;
  }

  public static class CountAndPlurals {
    private final int mCount;
    @PluralsRes
    private final int mPlurals;

    public CountAndPlurals(int count, int plurals)
    {
      mCount = count;
      mPlurals = plurals;
    }

    public int getCount()
    {
      return mCount;
    }

    public int getPlurals()
    {
      return mPlurals;
    }
  }

  public static class Author implements Parcelable
  {
    @NonNull
    private final String mId;
    @NonNull
    private final String mName;

    public Author(@NonNull String id, @NonNull String name)
    {
      mId = id;
      mName = name;
    }

    @NonNull
    public String getId()
    {
      return mId;
    }

    @NonNull
    public String getName()
    {
      return mName;
    }

    @Override
    public String toString()
    {
      final StringBuilder sb = new StringBuilder("Author{");
      sb.append("mId='").append(mId).append('\'');
      sb.append(", mName='").append(mName).append('\'');
      sb.append('}');
      return sb.toString();
    }

    @Override
    public boolean equals(Object o)
    {
      if (this == o) return true;
      if (o == null || getClass() != o.getClass()) return false;
      Author author = (Author) o;
      return mId.equals(author.mId);
    }

    @Override
    public int hashCode()
    {
      return mId.hashCode();
    }

    @Override
    public int describeContents()
    {
      return 0;
    }

    public static String getRepresentation(@NonNull Context context, @NonNull Author author)
    {
      Resources res = context.getResources();
      return String.format(res.getString(R.string.author_name_by_prefix), author.getName());
    }

    @Override
    public void writeToParcel(Parcel dest, int flags)
    {
      dest.writeString(this.mId);
      dest.writeString(this.mName);
    }

    protected Author(Parcel in)
    {
      this.mId = in.readString();
      this.mName = in.readString();
    }

    public static final Creator<Author> CREATOR = new Creator<Author>()
    {
      @Override
      public Author createFromParcel(Parcel source)
      {
        return new Author(source);
      }

      @Override
      public Author[] newArray(int size)
      {
        return new Author[size];
      }
    };
  }

  @Override
  public String toString()
  {
    final StringBuilder sb = new StringBuilder("BookmarkCategory{");
    sb.append("mId=").append(mId);
    sb.append(", mName='").append(mName).append('\'');
    sb.append(", mAuthor=").append(mAuthor);
    sb.append(", mAnnotation='").append(mAnnotation).append('\'');
    sb.append(", mDescription='").append(mDescription).append('\'');
    sb.append(", mTracksCount=").append(mTracksCount);
    sb.append(", mBookmarksCount=").append(mBookmarksCount);
    sb.append(", mType=").append(Type.values()[mTypeIndex]);
    sb.append(", mIsMyCategory=").append(mIsMyCategory);
    sb.append(", mIsVisible=").append(mIsVisible);
    sb.append(", mAccessRules=").append(getAccessRules());
    sb.append('}');
    return sb.toString();
  }

  public static class Downloaded implements TypeConverter<BookmarkCategory, Boolean>
  {
    @Override
    public Boolean convert(@NonNull BookmarkCategory data)
    {
      return data.getType() == Type.DOWNLOADED;
    }
  }

  public enum Type
  {
    PRIVATE(BookmarksPageFactory.PRIVATE, FilterStrategy.PredicativeStrategy.makePrivateInstance()),
    DOWNLOADED(BookmarksPageFactory.DOWNLOADED, FilterStrategy.PredicativeStrategy.makeDownloadedInstance());

    @NonNull
    private BookmarksPageFactory mFactory;
    @NonNull
    private FilterStrategy mFilterStrategy;

    Type(@NonNull BookmarksPageFactory pageFactory, @NonNull FilterStrategy filterStrategy)
    {
      mFactory = pageFactory;
      mFilterStrategy = filterStrategy;
    }

    @NonNull
    public BookmarksPageFactory getFactory()
    {
      return mFactory;
    }

    @NonNull
    public FilterStrategy getFilterStrategy()
    {
      return mFilterStrategy;
    }
  }

  @Override
  public int describeContents()
  {
    return 0;
  }

  @Override
  public void writeToParcel(Parcel dest, int flags)
  {
    dest.writeLong(this.mId);
    dest.writeString(this.mName);
    dest.writeParcelable(this.mAuthor, flags);
    dest.writeString(this.mAnnotation);
    dest.writeString(this.mDescription);
    dest.writeInt(this.mTracksCount);
    dest.writeInt(this.mBookmarksCount);
    dest.writeInt(this.mTypeIndex);
    dest.writeByte(this.mIsMyCategory ? (byte) 1 : (byte) 0);
    dest.writeByte(this.mIsVisible ? (byte) 1 : (byte) 0);
    dest.writeInt(this.mAccessRulesIndex);
  }

  protected BookmarkCategory(Parcel in)
  {
    this.mId = in.readLong();
    this.mName = in.readString();
    this.mAuthor = in.readParcelable(Author.class.getClassLoader());
    this.mAnnotation = in.readString();
    this.mDescription = in.readString();
    this.mTracksCount = in.readInt();
    this.mBookmarksCount = in.readInt();
    this.mTypeIndex = in.readInt();
    this.mIsMyCategory = in.readByte() != 0;
    this.mIsVisible = in.readByte() != 0;
    this.mAccessRulesIndex = in.readInt();
  }

  public static final Creator<BookmarkCategory> CREATOR = new Creator<BookmarkCategory>()
  {
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

  public enum AccessRules
  {
    ACCESS_RULES_LOCAL(R.string.not_shared, R.drawable.ic_lock),
    ACCESS_RULES_PUBLIC(R.string.public_access, R.drawable.ic_public_inline),
    ACCESS_RULES_DIRECT_LINK(R.string.limited_access, R.drawable.ic_link_inline),
    ACCESS_RULES_P2P(R.string.access_rules_p_to_p, R.drawable.ic_public_inline),
    ACCESS_RULES_PAID(R.string.access_rules_paid, R.drawable.ic_public_inline);

    private final int mResId;
    private final int mDrawableResId;

    AccessRules(int resId, int drawableResId)
    {
      mResId = resId;
      mDrawableResId = drawableResId;
    }

    @DrawableRes
    public int getDrawableResId()
    {
      return mDrawableResId;
    }

    @StringRes
    public int getNameResId()
    {
      return mResId;
    }
  }
}
