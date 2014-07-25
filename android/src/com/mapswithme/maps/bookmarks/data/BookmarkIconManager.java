package com.mapswithme.maps.bookmarks.data;

import java.util.HashMap;

public class BookmarkIconManager
{
  private static String[] ICONS = {
      "placemark-red", "placemark-blue", "placemark-purple", "placemark-yellow",
      "placemark-pink", "placemark-brown", "placemark-green", "placemark-orange"
  };


  static Icon getIcon(String type)
  {
    return new Icon(type, type);
  }

  static HashMap<String, Icon> getAll()
  {
    final HashMap<String, Icon> all = new HashMap<String, Icon>();
    for (int i = 0; i < ICONS.length; i++)
    {
      all.put(ICONS[i], getIcon(ICONS[i]));
    }
    return all;
  }
}
