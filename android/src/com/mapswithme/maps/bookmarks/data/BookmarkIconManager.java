package com.mapswithme.maps.bookmarks.data;

import java.util.HashMap;
import java.util.WeakHashMap;

import com.mapswithme.maps.R;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.util.Log;

public class BookmarkIconManager
{
  private Context mContext;
  private R.drawable mDrawables;
  private Bitmap mEmptyBitmap = null;
  private static String[] ICONS = {
             "placemark-red", "placemark-blue", "placemark-purple",
             "placemark-pink", "placemark-brown", "placemark-green", "placemark-orange"
         };
  private WeakHashMap<Integer, Bitmap> mIcons;

  public BookmarkIconManager(Context context)
  {
    mContext = context.getApplicationContext();
    mDrawables = new R.drawable();
    mIcons = new WeakHashMap<Integer, Bitmap>();
    mEmptyBitmap = BitmapFactory.decodeResource(context.getResources(), R.drawable.placemark_red);
  }

  Icon getIcon(String type)
  {
    Bitmap iconB = getIconBitmap(type);
    //Maybe we can find a better way to store icons?
    if (iconB == null)
    {
      throw new NullPointerException("I can't find icon! "+type);
    }
    return new Icon(type, type, iconB);
  }

  HashMap<String, Icon> getAll()
  {
    HashMap<String, Icon> all = new HashMap<String, Icon>();
    for (int i = 0; i < ICONS.length; i++)
    {
      all.put(ICONS[i], getIcon(ICONS[i]));
    }
    return all;
  }

  private int getIconDrawableId(String type)
  {
    try{
      Log.d("drawables ids", ""+mDrawables.getClass().getFields());
      Log.d("drawables type", ""+type);
    return mDrawables.getClass().getField(type.replace("-", "_")).getInt(mDrawables);
    }
    catch (IllegalAccessException e)
    {
      e.printStackTrace();
      return -1;
    }
    catch (NoSuchFieldException e)
    {
      e.printStackTrace();
      return -1;
    }
  }

  private Bitmap getIconBitmap(String type)
  {
    return Bitmap.createBitmap(mEmptyBitmap);
    /*
    try
    {
      int iconID = getIconDrawableId(type);
      if (iconID == -1)
      {
        return mEmptyBitmap;
      }
      Bitmap icon = null;
      icon = mIcons.get(iconID);
      if (icon==null)
      {
        //It's not take a lot of time to load this icon, so, now i'm not implementing icon loading in background thread.
        icon = mIcons.put(iconID, BitmapFactory.decodeResource(mContext.getResources(), iconID));
      }
      return icon;
    }
    catch (IllegalArgumentException e)
    {
      e.printStackTrace();
      return mEmptyBitmap;
    }*/
  }
}
