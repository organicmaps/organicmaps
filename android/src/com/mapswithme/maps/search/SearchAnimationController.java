package com.mapswithme.maps.search;

import android.animation.Animator;
import android.animation.ValueAnimator;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.view.View;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.util.UiUtils;

final class SearchAnimationController
{
  private static final int DURATION = MwmApplication.get().getResources().getInteger(R.integer.anim_menu);

  @NonNull
  private final View mToolbar;
  @NonNull
  private final View mTabBar;

  SearchAnimationController(@NonNull View toolbar, @NonNull View tabBar)
  {
    mToolbar = toolbar;
    mTabBar = tabBar;
  }

  void animate(final boolean show, @Nullable final Runnable completion)
  {
    if (mToolbar.getHeight() == 0 || mTabBar.getHeight() == 0)
    {
      mToolbar.post(new Runnable()
      {
        @Override
        public void run()
        {
          animate(show, completion);
        }
      });
      return;
    }
    final float translation = -mTabBar.getHeight() - mToolbar.getHeight();
    ValueAnimator animator =
        ValueAnimator.ofFloat(show ? translation: 0, show ? 0 : translation);
    animator.addUpdateListener(new ValueAnimator.AnimatorUpdateListener()
    {
      @Override
      public void onAnimationUpdate(ValueAnimator animation)
      {
        final float value = (float) animation.getAnimatedValue();
        mToolbar.setTranslationY(value);
        mTabBar.setTranslationY(value);
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
    animator.setDuration(DURATION);
    animator.start();
  }
}
