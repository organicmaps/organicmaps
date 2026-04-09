package app.organicmaps.sdk.bookmarks.data;

import androidx.annotation.Keep;
import androidx.annotation.MainThread;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

@MainThread
public final class BookmarkListSession implements AutoCloseable
{
  interface NativeBridge
  {
    long create(@NonNull BookmarkListSession session, long categoryId);
    void destroy(long nativePtr);
    void showDefault(long nativePtr);
    boolean search(long nativePtr, @NonNull String query);
    void sort(long nativePtr, int sortingType, boolean hasMyPosition, double lat, double lon);
    @NonNull
    BookmarkListRow getRow(long nativePtr, int index);
  }

  private static final NativeBridge NATIVE_BRIDGE = new NativeBridge() {
    @Override
    public long create(@NonNull BookmarkListSession session, long categoryId)
    {
      return nativeCreate(session, categoryId);
    }

    @Override
    public void destroy(long nativePtr)
    {
      nativeDestroy(nativePtr);
    }

    @Override
    public void showDefault(long nativePtr)
    {
      nativeShowDefault(nativePtr);
    }

    @Override
    public boolean search(long nativePtr, @NonNull String query)
    {
      return nativeSearch(nativePtr, query);
    }

    @Override
    public void sort(long nativePtr, int sortingType, boolean hasMyPosition, double lat, double lon)
    {
      nativeSort(nativePtr, sortingType, hasMyPosition, lat, lon);
    }

    @Override
    @NonNull
    public BookmarkListRow getRow(long nativePtr, int index)
    {
      return nativeGetRow(nativePtr, index);
    }
  };

  public interface Listener
  {
    void onBookmarkListSnapshotChanged(@NonNull BookmarkListSnapshot snapshot);
  }

  @NonNull
  private final NativeBridge mBridge;
  private long mNativePtr;
  private boolean mClosed;
  @NonNull
  private BookmarkListSnapshot mLatestSnapshot = BookmarkListSnapshot.EMPTY;
  @Nullable
  private Listener mListener;

  public BookmarkListSession(long categoryId)
  {
    this(categoryId, NATIVE_BRIDGE);
  }

  BookmarkListSession(long categoryId, @NonNull NativeBridge bridge)
  {
    mBridge = bridge;
    mNativePtr = mBridge.create(this, categoryId);
    if (mNativePtr == 0)
      throw new IllegalStateException("Failed to create native bookmark list session.");
  }

  public void setListener(@Nullable Listener listener)
  {
    mListener = listener;
    if (mListener != null)
      mListener.onBookmarkListSnapshotChanged(mLatestSnapshot);
  }

  @NonNull
  public BookmarkListSnapshot getLatestSnapshot()
  {
    return mLatestSnapshot;
  }

  /**
   * Fetches the full row content for the given position from the native session.
   * This is the lazy accessor called by the adapter during {@code onBindViewHolder}.
   */
  @NonNull
  public BookmarkListRow getRow(int index)
  {
    ensureOpen();
    return mBridge.getRow(mNativePtr, index);
  }

  public void showDefault()
  {
    ensureOpen();
    mBridge.showDefault(mNativePtr);
  }

  public boolean search(@NonNull String query)
  {
    ensureOpen();
    return mBridge.search(mNativePtr, query);
  }

  public void sort(@BookmarkCategory.SortingType int sortingType, boolean hasMyPosition, double lat, double lon)
  {
    ensureOpen();
    mBridge.sort(mNativePtr, sortingType, hasMyPosition, lat, lon);
  }

  @Override
  public void close()
  {
    if (mClosed)
      return;
    mClosed = true;
    mListener = null;
    if (mNativePtr != 0)
    {
      mBridge.destroy(mNativePtr);
      mNativePtr = 0;
    }
  }

  boolean isClosed()
  {
    return mClosed;
  }

  @Keep
  @SuppressWarnings("unused")
  void onSnapshotChanged(boolean loading, @Nullable int[] types, @Nullable long[] stableIds,
                         @Nullable int[] sectionKinds)
  {
    if (mClosed)
      return;
    if (types != null && stableIds != null && sectionKinds != null)
      mLatestSnapshot = new BookmarkListSnapshot(loading, types, stableIds, sectionKinds);
    else
      mLatestSnapshot = new BookmarkListSnapshot(loading, new int[0], new long[0], new int[0]);
    if (mListener != null)
      mListener.onBookmarkListSnapshotChanged(mLatestSnapshot);
  }

  private void ensureOpen()
  {
    if (mClosed || mNativePtr == 0)
      throw new IllegalStateException("BookmarkListSession is already closed.");
  }

  private static native long nativeCreate(@NonNull BookmarkListSession session, long categoryId);
  private static native void nativeDestroy(long nativePtr);
  private static native void nativeShowDefault(long nativePtr);
  private static native boolean nativeSearch(long nativePtr, @NonNull String query);
  private static native void nativeSort(long nativePtr, int sortingType, boolean hasMyPosition, double lat, double lon);
  @NonNull
  private static native BookmarkListRow nativeGetRow(long nativePtr, int index);
}
