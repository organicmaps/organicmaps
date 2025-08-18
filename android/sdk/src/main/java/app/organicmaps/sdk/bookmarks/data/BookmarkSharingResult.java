package app.organicmaps.sdk.bookmarks.data;

import androidx.annotation.IntDef;
import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

// Called from JNI.
@Keep
@SuppressWarnings("unused")
public class BookmarkSharingResult
{
  @Retention(RetentionPolicy.SOURCE)
  @IntDef({SUCCESS, EMPTY_CATEGORY, ARCHIVE_ERROR, FILE_ERROR})
  public @interface Code
  {}

  public static final int SUCCESS = 0;
  public static final int EMPTY_CATEGORY = 1;
  public static final int ARCHIVE_ERROR = 2;
  public static final int FILE_ERROR = 3;

  private final long[] mCategoriesIds;
  @Code
  private final int mCode;
  @NonNull
  private final String mSharingPath;
  @NonNull
  @SuppressWarnings("unused")
  private final String mErrorString;
  @NonNull
  @SuppressWarnings("unused")
  private final String mMimeType;

  public BookmarkSharingResult(long[] categoriesIds, @Code int code, @NonNull String sharingPath,
                               @NonNull String mimeType, @NonNull String errorString)
  {
    mCategoriesIds = categoriesIds;
    mCode = code;
    mSharingPath = sharingPath;
    mErrorString = errorString;
    mMimeType = mimeType;
  }

  public long[] getCategoriesIds()
  {
    return mCategoriesIds;
  }

  public int getCode()
  {
    return mCode;
  }

  @NonNull
  public String getSharingPath()
  {
    return mSharingPath;
  }
  @NonNull
  public String getMimeType()
  {
    return mMimeType;
  }

  @NonNull
  public String getErrorString()
  {
    return mErrorString;
  }
}
