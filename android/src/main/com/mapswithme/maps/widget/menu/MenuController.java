package com.mapswithme.maps.widget.menu;

import android.view.View;

import com.mapswithme.maps.base.Initializable;

public interface MenuController extends Initializable<View>
{
  void open();
  void close();
  boolean isClosed();
}
