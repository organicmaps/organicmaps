package com.mapswithme.maps.widget;

import android.graphics.drawable.Drawable;
import android.graphics.drawable.TransitionDrawable;

/**
 * Same as {@link TransitionDrawable} but correctly tracks transition direction.
 * I.e. if transition is in "start" state, {@link #reverseTransition(int)} does not start straight transition.
 */
public class TrackedTransitionDrawable extends TransitionDrawable
{
  private boolean mStart = true;


  public TrackedTransitionDrawable(Drawable[] layers)
  {
    super(layers);
  }

  @Override
  public void startTransition(int durationMillis)
  {
    if (!mStart)
      return;

    mStart = false;
    super.startTransition(durationMillis);
  }

  @Override
  public void reverseTransition(int duration)
  {
    if (mStart)
      return;

    mStart = true;
    super.reverseTransition(duration);
  }

  @Override
  public void resetTransition()
  {
    mStart = true;
    super.resetTransition();
  }
}
