package com.mapswithme.util;

import android.animation.Animator;
import androidx.annotation.IntDef;
import androidx.annotation.Nullable;
import android.view.View;
import android.view.ViewPropertyAnimator;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

public final class Animations
{
  private Animations() {}

  @IntDef({LEFT, RIGHT, TOP, BOTTOM})
  @Retention(RetentionPolicy.SOURCE)
  @interface AnimationDirection {}
  static final int LEFT = 0;
  static final int RIGHT = 1;
  static final int TOP = 2;
  public static final int BOTTOM = 3;

  private static final int DURATION_DEFAULT = MwmApplication.get().getResources().getInteger(R.integer.anim_default);

  public static void appearSliding(final View view, @AnimationDirection int appearFrom, @Nullable final Runnable completionListener)
  {
    if (UiUtils.isVisible(view))
    {
      if (completionListener != null)
        completionListener.run();
      return;
    }
    final ViewPropertyAnimator animator = view.animate().setDuration(DURATION_DEFAULT).alpha(1).setListener(new UiUtils.SimpleAnimatorListener()
    {
      @Override
      public void onAnimationEnd(Animator animation)
      {
        if (completionListener != null)
          completionListener.run();
      }
    });

    switch (appearFrom)
    {
      case LEFT:
      case RIGHT:
        animator.translationX(0);
        break;
      case TOP:
      case BOTTOM:
        animator.translationY(0);
        break;
    }

    UiUtils.show(view);
  }

  public static void disappearSliding(final View view, @AnimationDirection int disappearTo, @Nullable final Runnable completionListener)
  {
    if (!UiUtils.isVisible(view))
    {
      if (completionListener != null)
        completionListener.run();
      return;
    }

    final ViewPropertyAnimator animator = view.animate().setDuration(DURATION_DEFAULT).alpha(0).setListener(new UiUtils.SimpleAnimatorListener()
    {
      @Override
      public void onAnimationEnd(Animator animation)
      {
        UiUtils.hide(view);
        if (completionListener != null)
          completionListener.run();
      }
    });

    switch (disappearTo)
    {
      case RIGHT:
        animator.translationX(view.getWidth());
        break;
      case LEFT:
        animator.translationX(-view.getWidth());
        break;
      case BOTTOM:
        animator.translationY(view.getHeight());
        break;
      case TOP:
        animator.translationY(-view.getHeight());
        break;
    }
  }
}
