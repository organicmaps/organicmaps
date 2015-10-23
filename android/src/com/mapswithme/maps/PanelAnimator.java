package com.mapswithme.maps;

import android.animation.Animator;
import android.animation.ValueAnimator;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;
import android.view.View;
import android.view.animation.AccelerateInterpolator;

import com.mapswithme.util.UiUtils;

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
    mPanel.setTranslationX(offset);
    mPanel.setAlpha(offset / WIDTH + 1.0f);
    mAnimationTrackListener.onTrackLeftAnimation(offset + WIDTH);
  }

  /** @param completionListener will be called before the fragment becomes actually visible */
  public void show(final Class<? extends Fragment> clazz, final Bundle args, @Nullable final Runnable completionListener)
  {
    if (isVisible())
    {
      if (mActivity.getFragment(clazz) != null)
      {
        if (completionListener != null)
          completionListener.run();
        return;
      }

      hide(new Runnable()
      {
        @Override
        public void run()
        {
          show(clazz, args, completionListener);
        }
      });

      return;
    }

    mActivity.replaceFragmentInternal(clazz, args);
    if (completionListener != null)
      completionListener.run();

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
    animator.addListener(new UiUtils.SimpleAnimatorListener()
    {
      @Override
      public void onAnimationEnd(Animator animation)
      {
        mAnimationTrackListener.onTrackFinished(true);
        mActivity.adjustCompass(WIDTH, 0);
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
    animator.addListener(new UiUtils.SimpleAnimatorListener()
    {
      @Override
      public void onAnimationEnd(Animator animation)
      {
        UiUtils.hide(mPanel);
        mAnimationTrackListener.onTrackFinished(false);
        mActivity.adjustCompass(0, 0);

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
    return UiUtils.isVisible(mPanel);
  }
}
