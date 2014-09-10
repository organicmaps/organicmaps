package com.mapswithme.maps;

import android.graphics.drawable.AnimationDrawable;
import android.graphics.drawable.Drawable;
import android.widget.ImageButton;

import java.util.HashMap;
import java.util.Map;

public class LocationButtonImageSetter
{

  private final static Map<Integer, Integer> STATE_TO_RES = new HashMap<Integer, Integer>();

  static
  {
    STATE_TO_RES.put(LocationState.UNKNOWN_POSITION, R.drawable.ic_my_position);
    STATE_TO_RES.put(LocationState.NOT_FOLLOW, R.drawable.ic_my_position_pressed);
    STATE_TO_RES.put(LocationState.FOLLOW, R.drawable.ic_my_position_pressed);
    STATE_TO_RES.put(LocationState.ROTATE_AND_FOLLOW, R.drawable.ic_my_position_auto_follow);
    STATE_TO_RES.put(LocationState.PENDING_POSITION, R.drawable.ic_my_position_search);
  }

  public static void setButtonViewFromState(int state, ImageButton button)
  {
    final int id = STATE_TO_RES.get(state);
    final Drawable draw = button.getResources().getDrawable(id);

    button.setImageDrawable(draw);

    if (draw instanceof AnimationDrawable)
      ((AnimationDrawable) draw).start();
  }

}
