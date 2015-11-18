package com.mapswithme.maps.widget.menu;

import android.graphics.drawable.AnimationDrawable;
import android.graphics.drawable.Drawable;
import android.util.SparseIntArray;
import android.view.View;
import android.widget.ImageView;

import com.mapswithme.maps.R;
import com.mapswithme.util.statistics.AlohaHelper;
import com.mapswithme.util.statistics.Statistics;

import static com.mapswithme.maps.LocationState.*;

public class MyPositionButton
{
  private static final SparseIntArray STATE_RESOURCES = new SparseIntArray();

  private final ImageView mButton;

  static
  {
    STATE_RESOURCES.put(UNKNOWN_POSITION, R.drawable.ic_no_position);
    STATE_RESOURCES.put(NOT_FOLLOW, R.drawable.ic_not_follow);
    STATE_RESOURCES.put(FOLLOW, R.drawable.ic_follow);
    STATE_RESOURCES.put(ROTATE_AND_FOLLOW, R.drawable.ic_follow_and_rotate);
    STATE_RESOURCES.put(PENDING_POSITION, R.drawable.anim_myposition_pending);
  }

  public MyPositionButton(View button)
  {
    mButton = (ImageView) button;
    mButton.setOnClickListener(new View.OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        Statistics.INSTANCE.trackEvent(Statistics.EventName.TOOLBAR_MY_POSITION);
        AlohaHelper.logClick(AlohaHelper.TOOLBAR_MY_POSITION);
        INSTANCE.switchToNextMode();
      }
    });
  }

  @SuppressWarnings("deprecation")
  public void update(int state)
  {
    Drawable image = mButton.getResources().getDrawable(STATE_RESOURCES.get(state));
    mButton.setImageDrawable(image);

    if (image instanceof AnimationDrawable)
      ((AnimationDrawable) image).start();
  }
}
