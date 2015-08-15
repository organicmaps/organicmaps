package com.mapswithme.maps;

import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;
import android.view.View;
import android.view.animation.AccelerateInterpolator;

import com.mapswithme.util.UiUtils;
import com.nineoldandroids.animation.Animator;
import com.nineoldandroids.animation.ValueAnimator;
import com.nineoldandroids.view.ViewHelper;

class PanelAnimator
{
  private static final int DURATION = MwmApplication.get().getResources().getInteger(R.integer.anim_panel);
  private static final int WIDTH = UiUtils.dimen(R.dimen.panel_width);

  private final MwmActivity mActivity;
  private final MwmActivity.LeftAnimationTrackListener mAnimationTrackListener;
  private final View mPanel;


  public PanelAnimator(MwmActivity activity, @NonNull MwmActivity.LeftAnimationTrackListener animationTrackListener)
  {
    mActivity = activity;
    mAnimationTrackListener = animationTrackListener;
    mPanel = mActivity.findViewById(R.id.fragment_container);
  }

  private void track(ValueAnimator animation)
  {
    float offset = (Float) animation.getAnimatedValue();
    ViewHelper.setTranslationX(mPanel, offset);
    ViewHelper.setAlpha(mPanel, offset / WIDTH + 1.0f);
    mAnimationTrackListener.onTrackLeftAnimation(offset + WIDTH);
  }

  public void show(final Class<? extends Fragment> clazz, final Bundle args)
  {
    if (isVisible())
    {
      if (mActivity.getSupportFragmentManager().findFragmentByTag(clazz.getName()) != null)
        return;

      hide(new Runnable()
      {
        @Override
        public void run()
        {
          show(clazz, args);
        }
      });

      return;
    }

    mActivity.replaceFragmentInternal(clazz, args);

    UiUtils.show(mPanel);
    mAnimationTrackListener.onTrackStarted(false);

    ValueAnimator animator = ValueAnimator.ofFloat(-WIDTH, 0.0f);
    animator.addUpdateListener(new ValueAnimator.AnimatorUpdateListener()
    {
      @Override
      public void onAnimationUpdate(ValueAnimator animation)
      {
        track(animation);
      }
    });
    animator.addListener(new UiUtils.SimpleNineoldAnimationListener()
    {
      @Override
      public void onAnimationEnd(Animator animation)
      {
        mAnimationTrackListener.onTrackFinished(true);
        mActivity.adjustCompass(WIDTH);
      }
    });

    animator.setDuration(DURATION);
    animator.setInterpolator(new AccelerateInterpolator());
    animator.start();
  }

  public void hide(@Nullable final Runnable completionListener)
  {
    if (!isVisible())
    {
      if (completionListener != null)
        completionListener.run();
      return;
    }

    mAnimationTrackListener.onTrackStarted(true);

    ValueAnimator animator = ValueAnimator.ofFloat(0.0f, -WIDTH);
    animator.addUpdateListener(new ValueAnimator.AnimatorUpdateListener()
    {
      @Override
      public void onAnimationUpdate(ValueAnimator animation)
      {
        track(animation);
      }
    });
    animator.addListener(new UiUtils.SimpleNineoldAnimationListener()
    {
      @Override
      public void onAnimationEnd(Animator animation)
      {
        UiUtils.hide(mPanel);
        mAnimationTrackListener.onTrackFinished(false);
        mActivity.adjustCompass(0);

        if (completionListener != null)
          completionListener.run();
      }
    });

    animator.setDuration(DURATION);
    animator.setInterpolator(new AccelerateInterpolator());
    animator.start();
  }

  public boolean isVisible()
  {
    return (mPanel.getVisibility() == View.VISIBLE);
  }
}
