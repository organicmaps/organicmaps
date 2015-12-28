package com.mapswithme.maps.widget.menu;

import android.graphics.drawable.AnimationDrawable;
import android.graphics.drawable.Drawable;
import android.view.View;
import android.widget.ImageView;

import com.mapswithme.maps.LocationState;
import com.mapswithme.maps.R;
import com.mapswithme.util.Graphics;
import com.mapswithme.util.ThemeUtils;
import com.mapswithme.util.statistics.AlohaHelper;
import com.mapswithme.util.statistics.Statistics;

public class MyPositionButton
{
  private final ImageView mButton;

  MyPositionButton(View button)
  {
    mButton = (ImageView) button;
    mButton.setOnClickListener(new View.OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        Statistics.INSTANCE.trackEvent(Statistics.EventName.TOOLBAR_MY_POSITION);
        AlohaHelper.logClick(AlohaHelper.TOOLBAR_MY_POSITION);
        LocationState.INSTANCE.switchToNextMode();
      }
    });
  }

  @SuppressWarnings("deprecation")
  public void update(int state)
  {
    Drawable image;
    switch (state)
    {
    case LocationState.UNKNOWN_POSITION:
      image = Graphics.tint(mButton.getContext(), R.drawable.ic_follow, R.attr.iconTintLight);
      break;

    case LocationState.NOT_FOLLOW:
      image = Graphics.tint(mButton.getContext(), R.drawable.ic_not_follow);
      break;

    case LocationState.FOLLOW:
      image = Graphics.tint(mButton.getContext(), R.drawable.ic_follow, R.attr.colorAccent);
      break;

    case LocationState.ROTATE_AND_FOLLOW:
      image = Graphics.tint(mButton.getContext(), R.drawable.ic_follow_and_rotate, R.attr.colorAccent);
      break;

    case LocationState.PENDING_POSITION:
      image = mButton.getResources().getDrawable(ThemeUtils.getResource(mButton.getContext(), R.attr.myPositionButtonAnimation));
      break;

    default:
      throw new IllegalArgumentException("Invalid button state: " + state);
    }

    mButton.setImageDrawable(image);

    if (image instanceof AnimationDrawable)
      ((AnimationDrawable) image).start();
  }
}
