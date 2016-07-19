package com.mapswithme.maps.widget;

import android.animation.Animator;
import android.content.Context;
import android.support.annotation.NonNull;
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
  
  private final Animator.AnimatorListener mFadeInListener = new UiUtils.SimpleAnimatorListener()
  {
    @Override
    public void onAnimationEnd(Animator animation)
    {
      UiUtils.show(FadeView.this);
      animation.removeListener(this);
    }
  };

  private final Animator.AnimatorListener mFadeOutListener = new UiUtils.SimpleAnimatorListener()
  {
    @Override
    public void onAnimationEnd(Animator animation)
    {
      UiUtils.hide(FadeView.this);
      animation.removeListener(this);
    }
  };


  public interface Listener
  {
    boolean onTouch();
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
    setAlpha(0.0f);
    UiUtils.show(this);
    animate().alpha(FADE_ALPHA_VALUE)
             .setDuration(DURATION)
             .setListener(mFadeInListener)
             .start();
  }

  public void fadeOut()
  {
    setAlpha(FADE_ALPHA_VALUE);
    animate().alpha(0.0f)
             .setDuration(DURATION)
             .setListener(mFadeOutListener)
             .start();
  }

  @Override
  public boolean onTouchEvent(@NonNull MotionEvent event)
  {
    if (event.getAction() != MotionEvent.ACTION_DOWN)
      return true;

    if (mListener == null || mListener.onTouch())
      fadeOut();

    return true;
  }
}
