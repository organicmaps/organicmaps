package com.mapswithme.maps.routing;

import android.animation.Animator;
import android.animation.ValueAnimator;
import android.os.Bundle;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.mapswithme.maps.MwmActivity;
import com.mapswithme.maps.R;
import com.mapswithme.util.UiUtils;

public class RoutingPlanInplaceController extends RoutingPlanController
{
  @NonNull
  private final RoutingPlanListener mRoutingPlanListener;

  @Nullable
  private Animator mAnimator;

  public RoutingPlanInplaceController(@NonNull MwmActivity activity,
                                      @NonNull RoutingPlanListener routingPlanListener,
                                      @NonNull RoutingBottomMenuListener listener)
  {
    super(activity.findViewById(R.id.routing_plan_frame), activity, routingPlanListener, listener);
    mRoutingPlanListener = routingPlanListener;
  }

  public void show(final boolean show)
  {
    if (mAnimator != null)
    {
      mAnimator.cancel();
      mAnimator.removeAllListeners();
    }
    if (show)
      UiUtils.show(getFrame());

    mAnimator = animateFrame(show, new Runnable()
    {
      @Override
      public void run()
      {
        if (!show)
          UiUtils.hide(getFrame());
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

  @Nullable
  private ValueAnimator animateFrame(final boolean show, final @Nullable Runnable completion)
  {
    if (!checkFrameHeight())
    {
      getFrame().post(new Runnable()
      {
        @Override
        public void run()
        {
          animateFrame(show, completion);
        }
      });
      return null;
    }

    mRoutingPlanListener.onRoutingPlanStartAnimate(show);

    ValueAnimator animator =
        ValueAnimator.ofFloat(show ? -getFrame().getHeight() : 0, show ? 0 : -getFrame().getHeight());
    animator.addUpdateListener(new ValueAnimator.AnimatorUpdateListener()
    {
      @Override
      public void onAnimationUpdate(ValueAnimator animation)
      {
        getFrame().setTranslationY((Float) animation.getAnimatedValue());
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
    animator.setDuration(mAnimToggle);
    animator.start();
    return animator;
  }

  public interface RoutingPlanListener
  {
    void onRoutingPlanStartAnimate(boolean show);
  }
}
