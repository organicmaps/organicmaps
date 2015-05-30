package com.mapswithme.maps.widget;

import android.content.Context;
import android.support.annotation.NonNull;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.View;
import android.widget.FrameLayout;

import com.mapswithme.util.UiUtils;
import com.nineoldandroids.animation.Animator;
import com.nineoldandroids.animation.ObjectAnimator;

public class FadeView extends FrameLayout
{
  private static final float FADE_ALPHA_VALUE = 0.8f;
  private static final String PROPERTY_ALPHA = "alpha";

  private ObjectAnimator mFadeInAnimation;
  private ObjectAnimator mFadeOutAnimation;

  private Animator.AnimatorListener mFadeOutListener = new UiUtils.SimpleNineoldAnimationListener()
  {
    @Override
    public void onAnimationEnd(Animator animation)
    {
      setVisibility(View.GONE);
      if (mFadeListener != null)
        mFadeListener.onFadeOut();
    }
  };
  private Animator.AnimatorListener mFadeInListener = new UiUtils.SimpleNineoldAnimationListener()
  {
    @Override
    public void onAnimationEnd(Animator animation)
    {
      if (mFadeListener != null)
        mFadeListener.onFadeIn();
    }
  };

  public interface FadeListener
  {
    void onFadeOut();

    void onFadeIn();
  }

  private FadeListener mFadeListener;

  public FadeView(Context context)
  {
    super(context);
  }

  public FadeView(Context context, AttributeSet attrs)
  {
    super(context, attrs);
  }

  public FadeView(Context context, AttributeSet attrs, int defStyleAttr)
  {
    super(context, attrs, defStyleAttr);
  }

  public void setFadeListener(FadeListener listener)
  {
    mFadeListener = listener;
  }

  public void fadeIn()
  {
    setVisibility(View.VISIBLE);
    mFadeInAnimation = ObjectAnimator.ofFloat(this, PROPERTY_ALPHA, 0f, FADE_ALPHA_VALUE);
    mFadeInAnimation.addListener(mFadeInListener);
    mFadeInAnimation.start();
  }

  public void fadeOut()
  {
    mFadeOutAnimation = ObjectAnimator.ofFloat(this, PROPERTY_ALPHA, FADE_ALPHA_VALUE, 0f);
    mFadeOutAnimation.addListener(mFadeOutListener);
    mFadeOutAnimation.start();
  }

  public boolean isFadingIn()
  {
    return mFadeInAnimation != null && mFadeInAnimation.isRunning();
  }

  public boolean isFadingOut()
  {
    return mFadeOutAnimation != null && mFadeOutAnimation.isRunning();
  }

  @Override
  public boolean onTouchEvent(@NonNull MotionEvent event)
  {
    if (event.getAction() == MotionEvent.ACTION_DOWN)
      fadeOut();

    return true;
  }
}
