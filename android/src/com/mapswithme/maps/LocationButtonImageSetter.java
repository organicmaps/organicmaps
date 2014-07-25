package com.mapswithme.maps;

import android.graphics.drawable.AnimationDrawable;
import android.graphics.drawable.Drawable;
import android.widget.ImageButton;

import java.util.HashMap;
import java.util.Map;

public class LocationButtonImageSetter
{
  public enum ButtonState
  {
    NO_LOCATION,
    WAITING_LOCATION,
    HAS_LOCATION,
    FOLLOW_MODE
  }

  private final static Map<ButtonState, Integer> STATE_TO_RES = new HashMap<ButtonState, Integer>();

  static
  {
    STATE_TO_RES.put(ButtonState.NO_LOCATION, R.drawable.ic_my_position);
    STATE_TO_RES.put(ButtonState.HAS_LOCATION, R.drawable.ic_my_position_pressed);
    STATE_TO_RES.put(ButtonState.FOLLOW_MODE, R.drawable.ic_my_position_auto_follow);
    STATE_TO_RES.put(ButtonState.WAITING_LOCATION, R.drawable.ic_my_position_search);
  }

  public static void setButtonViewFromState(ButtonState state, ImageButton button)
  {
    final int id = STATE_TO_RES.get(state);
    final Drawable draw = button.getResources().getDrawable(id);

    button.setImageDrawable(draw);

    if (draw instanceof AnimationDrawable)
      ((AnimationDrawable) draw).start();
  }

}
