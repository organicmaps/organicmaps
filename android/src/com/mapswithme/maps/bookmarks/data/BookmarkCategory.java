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

  private native boolean nIsVisible(int id);
  private native void nSetVisibility(int id, boolean v);

  private native String nGetName(int id);
  private native void nSetName(int old, String n);

  private native int nGetSize(int id);

  public int getId()
  {
    return mId;
  }

  public String getName()
  {
    return mName == null? nGetName(mId): mName;
  }

  public boolean isVisible()
  {
    return nIsVisible(mId);
  }

  public void setVisibility(boolean b)
  {
    nSetVisibility(mId, b);
  }

  public void setName(String name)
  {
    nSetName(mId, name);
    mName = name;
  }

  public int getSize()
  {
    return nGetSize(mId);
  }

  public Bookmark getBookmark(int b)
  {
    return new Bookmark(mContext, mId, b);
  }
}
