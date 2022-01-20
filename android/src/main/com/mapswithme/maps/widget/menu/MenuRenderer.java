package com.mapswithme.maps.widget.menu;

import android.view.View;

import com.mapswithme.maps.base.Hideable;
import com.mapswithme.maps.base.Initializable;

public interface MenuRenderer extends Initializable<View>, Hideable
{
  void render();
}
