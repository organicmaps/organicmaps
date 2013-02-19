package com.mapswithme.maps.bookmarks.data;

import android.content.Context;


public class BookmarkCategory
{
  private int mId;
  private String mName;
  private Context mContext;
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
    return getSize(mId);
  }

  public Bookmark getBookmark(int b)
  {
    return new Bookmark(mContext, mId, b);
  }
}
