package com.mapswithme.maps.bookmarks.data;

import java.util.HashMap;
import java.util.WeakHashMap;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;

import com.mapswithme.maps.R;

public class BookmarkIconManager
{
  private Context mContext;
  private Bitmap mEmptyBitmap = null;
  private static String[] ICONS = {
             "placemark-red", "placemark-blue", "placemark-purple", "placemark-yellow",
             "placemark-pink", "placemark-brown", "placemark-green", "placemark-orange"
         };
  private WeakHashMap<String, Bitmap> mIcons;

  public BookmarkIconManager(Context context)
  {
    mContext = context.getApplicationContext();
    mIcons = new WeakHashMap<String, Bitmap>();
    mEmptyBitmap = BitmapFactory.decodeResource(context.getResources(), R.drawable.placemark_red);
  }

  Icon getIcon(String type)
  {
    Bitmap iconB = getIconBitmap(type);
    //Maybe we can find a better way to store icons?
    if (iconB == null)
    {
      return new Icon("placemark-red", "placemark-red", mEmptyBitmap);
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

  private Bitmap getIconBitmap(String type)
  {
    try
    {

      Bitmap icon = null;
      icon = mIcons.get(type);
      if (icon==null)
      {
        //It's not take a lot of time to load this icon, so, now i'm not implementing icon loading in background thread.
        mIcons.put(type, icon = BitmapFactory.decodeResource(
                                        mContext.getResources(),
                                        mContext.getResources().
                                          getIdentifier(type.replace("-", "_"), "drawable", mContext.getPackageName())
                                          )
                         );
      }
      return icon;
    }
    catch (Exception e)
    {
      return mEmptyBitmap;
    }
  }
}
