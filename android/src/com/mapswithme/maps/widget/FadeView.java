package com.mapswithme.maps.widget;

import android.animation.Animator;
import android.animation.ObjectAnimator;
import android.content.Context;
import android.support.annotation.NonNull;
import android.support.v4.view.ViewCompat;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.widget.FrameLayout;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.util.UiUtils;

public class FadeView extends FrameLayout
{
  private static final float FADE_ALPHA_VALUE = 0.4f;
  private static final String PROPERTY_ALPHA = "alpha";
  private static final int DURATION = MwmApplication.get().getResources().getInteger(R.integer.anim_fade_main);
  
  private final Animator.AnimatorListener mFadeOutListener = new UiUtils.SimpleAnimatorListener()
  {
    @Override
    public void onAnimationEnd(Animator animation)
    {
      UiUtils.hide(FadeView.this);
    }
  };


  public interface Listener
  {
    void onTouch();
  }

  private Listener mListener;

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

  public void setListener(Listener listener)
  {
    mListener = listener;
  }

  public void fadeIn()
  {
    UiUtils.show(this);

    ObjectAnimator animation = ObjectAnimator.ofFloat(this, PROPERTY_ALPHA, 0f, FADE_ALPHA_VALUE);
    animation.setDuration(DURATION);
    animation.start();
  }

  public void fadeOut(boolean notify)
  {
    if (mListener != null && notify)
      mListener.onTouch();

    ObjectAnimator animation = ObjectAnimator.ofFloat(this, PROPERTY_ALPHA, FADE_ALPHA_VALUE, 0f);
    animation.addListener(mFadeOutListener);
    animation.setDuration(DURATION);
    animation.start();
  }

  public void fadeInInstantly()
  {
    UiUtils.show(this);
    setAlpha(FADE_ALPHA_VALUE);
  }

  @Override
  public boolean onTouchEvent(@NonNull MotionEvent event)
  {
    if (event.getAction() == MotionEvent.ACTION_DOWN)
      fadeOut(true);

    return true;
  }
}
