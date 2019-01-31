package com.mapswithme.maps.widget.placepage;

import android.os.Bundle;
import android.support.annotation.NonNull;

import com.mapswithme.maps.base.Initializable;
import com.mapswithme.maps.base.Savable;
import com.mapswithme.maps.bookmarks.data.MapObject;

public interface PlacePageController extends Initializable, Savable<Bundle>
{
  void openFor(@NonNull MapObject object);
  void close();
  // TODO: probably this method is redundant
  boolean isClosed();
}
