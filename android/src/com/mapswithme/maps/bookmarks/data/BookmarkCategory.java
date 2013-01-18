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
    mName = getName();
  }

  BookmarkCategory(Context c, String name)
  {
    mContext = c;
    mName = name;
  }

  private native boolean nIsVisible(String name);
  private native void nSetVisibility(String name, boolean v);

  private native String nGetName(int id);
  private native void nSetName(String old, String n);

  private native int nGetSize(String name);

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
    return nIsVisible(getName());
  }

  public void setVisibility(boolean b)
  {
    nSetVisibility(getName(), b);
  }

  public void setName(String name)
  {
    nSetName(mName, name);
    mName = name;
  }

  public int getSize()
  {
    return nGetSize(getName());
  }

  public Bookmark getBookmark(int b)
  {
    return new Bookmark(mContext, getName(), b);
  }

  void addPin(Bookmark pin)
  {
    //mPins.add(pin);
  }

  void removePin(Bookmark pin)
  {
    //mPins.remove(pin);
  }
}
