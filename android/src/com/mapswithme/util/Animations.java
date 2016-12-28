package com.mapswithme.util;

import android.animation.Animator;
import android.animation.ValueAnimator;
import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.Rect;
import android.support.annotation.IntDef;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.content.ContextCompat;
import android.util.DisplayMetrics;
import android.view.View;
import android.view.ViewGroup;
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
  public @interface AnimationDirection {}
  public static final int LEFT = 0;
  public static final int RIGHT = 1;
  public static final int TOP = 2;
  public static final int BOTTOM = 3;

  private static final int DURATION_DEFAULT = MwmApplication.get().getResources().getInteger(R.integer.anim_default);
  private static final int DURATION_MENU = MwmApplication.get().getResources().getInteger(R.integer.anim_menu);

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

  public static void exchangeViews(final View viewToHide, final @AnimationDirection int disappearTo,
                                   final View viewToShow, final @AnimationDirection int appearFrom, @Nullable final Runnable completionListener)
  {
    disappearSliding(viewToHide, disappearTo, new Runnable()
    {
      @Override
      public void run()
      {
        appearSliding(viewToShow, appearFrom, completionListener);
      }
    });
  }

  public static void fadeOutView(@NonNull final View view, @Nullable final Runnable completionListener)
  {
    view.animate().setDuration(DURATION_DEFAULT).alpha(0).setListener(new UiUtils.SimpleAnimatorListener() {
      @Override
      public void onAnimationEnd(Animator animation)
      {
        if (completionListener != null)
          completionListener.run();
      }
    });
  }

  public static void fadeInView(@NonNull final View view, @Nullable final Runnable completionListener)
  {
    view.animate().setDuration(DURATION_DEFAULT).alpha(1).setListener(new UiUtils.SimpleAnimatorListener() {
      @Override
      public void onAnimationEnd(Animator animation)
      {
        if (completionListener != null)
          completionListener.run();
      }
    });
  }

  public static void riseTransition(@NonNull final ViewGroup rootView, @NonNull final Rect startRect,
                                    @Nullable final Runnable runnable)
  {
    final Context context = rootView.getContext();
    final View view = new View(context);
    setCardBackgroundColor(view);

    DisplayMetrics dm = context.getResources().getDisplayMetrics();
    final float screenWidth = dm.widthPixels;
    final float screenHeight = dm.heightPixels;
    final float width = startRect.width();
    final float height = startRect.height();
    ViewGroup.MarginLayoutParams lp = new ViewGroup.MarginLayoutParams((int) width,
                                                                       (int) height);
    lp.topMargin = startRect.top;
    lp.leftMargin = startRect.left;
    final float right = screenWidth - startRect.right;
    lp.rightMargin = (int) right;
    rootView.addView(view, lp);

    ValueAnimator animator = ValueAnimator.ofFloat(0.0f, 1.0f);
    animator.addUpdateListener(new ValueAnimator.AnimatorUpdateListener()
    {
      @Override
      public void onAnimationUpdate(ValueAnimator animation)
      {
        final float t = (float) animation.getAnimatedValue();
        final float topMargin = startRect.top - t * startRect.top;
        final float leftMargin = startRect.left - t * startRect.left;
        final float rightMargin = right - t * right;
        final float newWidth = width + t * (screenWidth - width);
        final float newHeight = height + t * (screenHeight - height);
        ViewGroup.MarginLayoutParams lp = (ViewGroup.MarginLayoutParams) view.getLayoutParams();
        lp.width = (int) newWidth;
        lp.height = (int) newHeight;
        lp.topMargin = (int) topMargin;
        lp.leftMargin = (int) leftMargin;
        lp.rightMargin = (int) rightMargin;
        view.setLayoutParams(lp);
      }
    });
    animator.addListener(new UiUtils.SimpleAnimatorListener()
    {
      @Override
      public void onAnimationEnd(Animator animation)
      {
        if (runnable != null)
          runnable.run();
        rootView.postDelayed(new Runnable()
        {
          @Override
          public void run()
          {
            rootView.removeView(view);
          }
        }, context.getResources().getInteger(android.R.integer.config_longAnimTime));
      }
    });
    animator.setDuration(DURATION_MENU);
    animator.start();
  }

  private static void setCardBackgroundColor(@NonNull View view)
  {
    Context context = view.getContext();
    TypedArray a = null;
    try
    {
      a = context.obtainStyledAttributes(new int[] { R.attr.cardBackgroundColor });
      int color = a.getColor(0, ContextCompat.getColor(context, R.color.bg_cards));
      view.setBackgroundColor(color);
    }
    finally
    {
      if (a != null)
        a.recycle();
    }
  }
}
