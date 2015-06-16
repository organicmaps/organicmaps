package com.mapswithme.maps.widget;

import android.content.Context;
import android.os.Build;
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
      // IMPORTANT views after alpha animations with 'setFillAfter' on 2.3 can't become GONE, until clearAnimation is called.
      if (Build.VERSION.SDK_INT < Build.VERSION_CODES.HONEYCOMB)
        clearAnimation();
      if (mFadeListener != null && mDoNotify)
        mFadeListener.onFadeOut();
    }
  };
  private Animator.AnimatorListener mFadeInListener = new UiUtils.SimpleNineoldAnimationListener()
  {
    @Override
    public void onAnimationEnd(Animator animation)
    {
      if (mFadeListener != null && mDoNotify)
        mFadeListener.onFadeIn();
    }
  };
  private boolean mDoNotify;

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

  /**
   * Fades out view and notifies on animation end, if requested
   * @param notify whether we want notification
   */
  public void fadeIn(boolean notify)
  {
    mDoNotify = notify;
    setVisibility(View.VISIBLE);
    mFadeInAnimation = ObjectAnimator.ofFloat(this, PROPERTY_ALPHA, 0f, FADE_ALPHA_VALUE);
    mFadeInAnimation.addListener(mFadeInListener);
    mFadeInAnimation.start();
  }

  /**
   * Fades out view and notifies on animation end, if requested
   * @param notify whether we want notification
   */
  public void fadeOut(boolean notify)
  {
    mDoNotify = notify;
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
      fadeOut(true);

    return true;
  }
}
