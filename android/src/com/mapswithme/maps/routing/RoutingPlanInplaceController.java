package com.mapswithme.maps.routing;

import android.animation.Animator;
import android.animation.ValueAnimator;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import com.mapswithme.maps.MwmActivity;
import com.mapswithme.maps.R;
import com.mapswithme.util.UiUtils;

public class RoutingPlanInplaceController extends RoutingPlanController
{
  @Nullable
  private RoutingPlanListener mRoutingPlanListener;

  public RoutingPlanInplaceController(@NonNull MwmActivity activity,
                                      @Nullable RoutingPlanListener routingPlanListener,
                                      @Nullable RoutingBottomMenuListener listener)
  {
    super(activity.findViewById(R.id.routing_plan_frame), activity, listener);
    mRoutingPlanListener = routingPlanListener;
  }

  public void show(final boolean show)
  {
    if (show)
      UiUtils.show(mFrame);

    animateFrame(show, new Runnable()
    {
      @Override
      public void run()
      {
        if (!show)
          UiUtils.hide(mFrame);
      }
    });
  }

  public void onSaveState(@NonNull Bundle outState)
  {
    saveRoutingPanelState(outState);
  }

  public void restoreState(@NonNull Bundle state)
  {
    restoreRoutingPanelState(state);
  }

  private void animateFrame(final boolean show, final @Nullable Runnable completion)
  {
    if (!checkFrameHeight())
    {
      mFrame.post(new Runnable()
      {
        @Override
        public void run()
        {
          animateFrame(show, completion);
        }
      });
      return;
    }

    if (mRoutingPlanListener != null)
      mRoutingPlanListener.onRoutingPlanStartAnimate(show);

    ValueAnimator animator =
        ValueAnimator.ofFloat(show ? -mFrameHeight : 0, show ? 0 : -mFrameHeight);
    animator.addUpdateListener(new ValueAnimator.AnimatorUpdateListener()
    {
      @Override
      public void onAnimationUpdate(ValueAnimator animation)
      {
        mFrame.setTranslationY((Float) animation.getAnimatedValue());
      }
    });
    animator.addListener(new UiUtils.SimpleAnimatorListener()
    {
      @Override
      public void onAnimationEnd(Animator animation)
      {
        if (completion != null)
          completion.run();
      }
    });
    animator.setDuration(ANIM_TOGGLE);
    animator.start();
  }

  public interface RoutingPlanListener
  {
    void onRoutingPlanStartAnimate(boolean show);
  }
}
