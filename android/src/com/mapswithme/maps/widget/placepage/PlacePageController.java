package com.mapswithme.maps.widget.placepage;

import android.support.annotation.NonNull;

import com.mapswithme.maps.base.Initializable;
import com.mapswithme.maps.bookmarks.data.MapObject;

public interface PlacePageController extends Initializable
{
  void openFor(@NonNull MapObject object);
  void close();
  // TODO: probably this method is redundant
  boolean isClosed();
}
