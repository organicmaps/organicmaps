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
    return (mName == null ? getName(mId) : mName);
  }

  public boolean isVisible()
  {
    return isVisible(mId);
  }

  public void setVisibility(boolean b)
  {
    setVisibility(mId, b);
  }

  public void setName(String name)
  {
    setName(mId, name);
    mName = name;
  }

  public int getSize()
  {
    return getSize(mId);
  }

  public int getBookmarksCount()
  {
    return getBookmarksCount(mId);
  }

  public int getTracksCount()
  {
    return getTracksCount(mId);
  }

  public Bookmark getBookmark(int index)
  {
    return getBookmark(mId, index, Bookmark.class);
  }

  public Track getTrack(int index)
  {
    return getTrack(mId, index, Track.class);
  }

  private native int getBookmarksCount(int id);

  private native int getTracksCount(int id);

  private native int getSize(int id);

  private native Bookmark getBookmark(int id, int index, Class<Bookmark> bookmarkClazz);

  private native Track getTrack(int id, int index, Class<Track> trackClazz);

  private native boolean isVisible(int id);

  private native void setVisibility(int id, boolean v);

  private native String getName(int id);

  private native void setName(int old, String n);
}
