package com.mapswithme.maps.bookmarks.data;

public class BookmarkCategory
{
  private final int mId;
  private String mName;

  BookmarkCategory(int id)
  {
    mId = id;
  }

  public int getId()
  {
    return mId;
  }

  public String getName()
  {
    return (mName == null ? nativeGetName(mId) : mName);
  }

  public void setName(String name)
  {
    nativeSetName(mId, name);
    mName = name;
  }

  public boolean isVisible()
  {
    return nativeIsVisible(mId);
  }

  public void setVisibility(boolean visible)
  {
    nativeSetVisibility(mId, visible);
  }

  /**
   * @return total count - tracks + bookmarks
   */
  public int getSize()
  {
    return nativeGetSize(mId);
  }

  public int getBookmarksCount()
  {
    return nativeGetBookmarksCount(mId);
  }

  public int getTracksCount()
  {
    return nativeGetTracksCount(mId);
  }

  public Bookmark getBookmark(int bookmarkId)
  {
    return nativeGetBookmark(mId, bookmarkId);
  }

  public Track nativeGetTrack(int trackId)
  {
    return nativeGetTrack(mId, trackId, Track.class);
  }

  private native int nativeGetBookmarksCount(int id);

  private native int nativeGetTracksCount(int id);

  private native int nativeGetSize(int id);

  private native Bookmark nativeGetBookmark(int catId, int bmkId);

  private native Track nativeGetTrack(int catId, int bmkId, Class<Track> trackClazz);

  private native boolean nativeIsVisible(int id);

  private native void nativeSetVisibility(int id, boolean visible);

  private native String nativeGetName(int id);

  private native void nativeSetName(int id, String n);
}
