package com.mapswithme.maps.bookmarks.data;

import android.content.Context;


public class BookmarkCategory
{
  private final int mId;
  private String mName;
  private final Context mContext;
  BookmarkCategory(Context c, int id)
  {
    mContext = c;
    mId = id;
  }

  private native boolean isVisible(int id);
  private native void setVisibility(int id, boolean v);

  private native String getName(int id);
  private native void setName(int old, String n);

  private native int getSize(int id);

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
    return getBookmarksCount() + getTracksCount();
  }

  public int getBookmarksCount()
  {
    return getSize(mId);
  }

  public int getTracksCount()
  {
    return 3; //TODO add native
  }

  public Bookmark getBookmark(int b)
  {
    return new Bookmark(mContext, mId, b - 3); //TODO remove - 3
  }

  public Track getTrack(int index)
  {
    return new Track(); // TODO: add native
  }
}
