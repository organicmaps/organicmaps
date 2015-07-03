package com.mapswithme.maps;

import android.graphics.drawable.AnimationDrawable;
import android.graphics.drawable.Drawable;
import android.widget.ImageButton;

import java.util.HashMap;
import java.util.Map;

public class LocationButtonImageSetter
{
  private final static Map<Integer, Integer> STATE_TO_RES_MAP = new HashMap<>();

  static
  {
    STATE_TO_RES_MAP.put(LocationState.UNKNOWN_POSITION, R.drawable.ic_no_position);
    STATE_TO_RES_MAP.put(LocationState.NOT_FOLLOW, R.drawable.ic_not_follow);
    STATE_TO_RES_MAP.put(LocationState.FOLLOW, R.drawable.ic_follow);
    STATE_TO_RES_MAP.put(LocationState.ROTATE_AND_FOLLOW, R.drawable.ic_follow_and_rotate);
    STATE_TO_RES_MAP.put(LocationState.PENDING_POSITION, R.drawable.anim_myposition_pending);
  }

  @SuppressWarnings("deprecation")
  public static void setButtonViewFromState(int state, ImageButton button)
  {
    final int id = STATE_TO_RES_MAP.get(state);
    final Drawable drawable = button.getResources().getDrawable(id);

    button.setImageDrawable(drawable);

    if (drawable instanceof AnimationDrawable)
      ((AnimationDrawable) drawable).start();
  }
}
