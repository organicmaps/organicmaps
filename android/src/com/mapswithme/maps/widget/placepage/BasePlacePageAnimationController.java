package com.mapswithme.maps.widget.placepage;

import android.animation.TimeInterpolator;
import android.animation.ValueAnimator;
import android.support.annotation.NonNull;
import android.support.v4.view.GestureDetectorCompat;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewConfiguration;
import android.view.ViewGroup;
import android.view.animation.AccelerateInterpolator;
import android.view.animation.Interpolator;
import android.widget.ScrollView;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.maps.widget.placepage.PlacePageView.State;
import com.mapswithme.util.UiUtils;

/**
 * Class is responsible for animations of PP(place page) and PPP(place page preview).
 */
public abstract class BasePlacePageAnimationController
{
  protected static final int DURATION = MwmApplication.get().getResources().getInteger(R.integer.anim_placepage);
  protected static final TimeInterpolator INTERPOLATOR = new AccelerateInterpolator();
  protected State mState = State.HIDDEN;

  protected PlacePageView mPlacePage;
  protected ViewGroup mPreview;
  protected ViewGroup mDetailsFrame;
  protected ScrollView mDetailsScroll;
  protected View mDetailsContent;
  protected ViewGroup mBookmarkDetails;
  protected ViewGroup mButtons;
  // Gestures
  protected GestureDetectorCompat mGestureDetector;
  protected boolean mIsGestureHandled;
  protected float mDownCoord;
  protected float mTouchSlop;
  // Visibility
  protected OnVisibilityChangedListener mVisibilityChangedListener;

  public interface OnVisibilityChangedListener
  {
    void onPreviewVisibilityChanged(boolean isVisible);
    void onPlacePageVisibilityChanged(boolean isVisible);
  }

  protected abstract void initGestureDetector();

  public BasePlacePageAnimationController(@NonNull PlacePageView placePage)
  {
    mPlacePage = placePage;
    mPreview = (ViewGroup) placePage.findViewById(R.id.pp__preview);
    mDetailsFrame = (ViewGroup) placePage.findViewById(R.id.pp__details_frame);
    mDetailsScroll = (ScrollView) placePage.findViewById(R.id.pp__details);
    mDetailsContent = mDetailsScroll.getChildAt(0);
    mBookmarkDetails = (ViewGroup) mDetailsFrame.findViewById(R.id.bookmark_frame);
    mButtons = (ViewGroup) placePage.findViewById(R.id.pp__buttons);
    initGestureDetector();

    mTouchSlop = ViewConfiguration.get(mPlacePage.getContext()).getScaledTouchSlop();

    if (mPlacePage.isFloating() || mPlacePage.isDocked())
      mDetailsFrame.setPadding(mDetailsFrame.getPaddingLeft(), mDetailsFrame.getPaddingTop(), mDetailsFrame.getPaddingRight(),
                               UiUtils.dimen(R.dimen.place_page_buttons_height));

    if (mPlacePage.isDocked())
    {
      ViewGroup.LayoutParams lp = mDetailsFrame.getLayoutParams();
      lp.height = ViewGroup.LayoutParams.MATCH_PARENT;
      mDetailsFrame.setLayoutParams(lp);
    }

    initialVisibility();
  }

  public State getState()
  {
    return mState;
  }

  public void setState(final State state, @MapObject.MapObjectType final int type)
  {
    mPlacePage.post(new Runnable() {
      @Override
      public void run()
      {
        onStateChanged(mState, state, type);
        mState = state;
      }
    });
  }

  protected void initialVisibility()
  {
    UiUtils.invisible(mPlacePage, mPreview, mDetailsFrame, mBookmarkDetails);
  }

  protected abstract void onStateChanged(State currentState, State newState, int type);

  public void setOnVisibilityChangedListener(OnVisibilityChangedListener listener)
  {
    mVisibilityChangedListener = listener;
  }

  protected abstract boolean onInterceptTouchEvent(MotionEvent event);

  protected boolean onTouchEvent(@NonNull MotionEvent event)
  {
    return mGestureDetector.onTouchEvent(event);
  }

  protected void notifyVisibilityListener(boolean previewShown, boolean ppShown)
  {
    if (mVisibilityChangedListener != null)
    {
      mVisibilityChangedListener.onPreviewVisibilityChanged(previewShown);
      mVisibilityChangedListener.onPlacePageVisibilityChanged(ppShown);
    }
  }

  protected void startDefaultAnimator(ValueAnimator animator)
  {
    startDefaultAnimator(animator, new AccelerateInterpolator());
  }

  protected void startDefaultAnimator(ValueAnimator animator, Interpolator interpolator)
  {
    animator.setDuration(DURATION);
    animator.setInterpolator(interpolator == null ? INTERPOLATOR : interpolator);
    animator.start();
  }
}
