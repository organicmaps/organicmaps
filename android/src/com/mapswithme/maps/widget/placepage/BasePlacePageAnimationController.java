package com.mapswithme.maps.widget.placepage;

import android.animation.TimeInterpolator;
import android.animation.ValueAnimator;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.view.GestureDetectorCompat;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewConfiguration;
import android.view.ViewGroup;
import android.view.animation.AccelerateInterpolator;
import android.view.animation.Interpolator;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.maps.widget.ObservableScrollView;
import com.mapswithme.maps.widget.placepage.PlacePageView.State;
import com.mapswithme.util.UiUtils;

/**
 * Class is responsible for animations of PP(place page) and PPP(place page preview).
 */
public abstract class BasePlacePageAnimationController implements ObservableScrollView.ScrollListener
{
  private static final int DURATION = MwmApplication.get()
                                                    .getResources()
                                                    .getInteger(R.integer.anim_placepage);
  private static final TimeInterpolator INTERPOLATOR = new AccelerateInterpolator();
  State mState = State.HIDDEN;

  PlacePageView mPlacePage;
  ViewGroup mPreview;
  ViewGroup mDetailsFrame;
  ObservableScrollView mDetailsScroll;
  View mDetailsContent;
  ViewGroup mBookmarkDetails;
  ViewGroup mButtons;
  // Gestures
  GestureDetectorCompat mGestureDetector;
  boolean mIsDragging;
  float mDownCoord;
  float mTouchSlop;
  int mCurrentScrollY;

  private OnVisibilityChangedListener mVisibilityListener;

  public interface OnVisibilityChangedListener
  {

    void onPreviewVisibilityChanged(boolean isVisible);

    void onPlacePageVisibilityChanged(boolean isVisible);
  }

  @Nullable
  private OnAnimationListener mProgressListener;

  public interface OnAnimationListener
  {
    void onProgress(float translationX, float translationY);
  }

  protected abstract void initGestureDetector();

  BasePlacePageAnimationController(@NonNull PlacePageView placePage)
  {
    mPlacePage = placePage;
    mPreview = (ViewGroup) placePage.findViewById(R.id.pp__preview);
    mDetailsFrame = (ViewGroup) placePage.findViewById(R.id.pp__details_frame);
    mDetailsScroll = (ObservableScrollView) placePage.findViewById(R.id.pp__details);
    mDetailsScroll.setScrollListener(this);
    mDetailsContent = mDetailsScroll.getChildAt(0);
    mBookmarkDetails = (ViewGroup) mDetailsFrame.findViewById(R.id.bookmark_frame);
    mButtons = (ViewGroup) placePage.findViewById(R.id.pp__buttons);
    initGestureDetector();

    mTouchSlop = ViewConfiguration.get(mPlacePage.getContext()).getScaledTouchSlop();

    if (mPlacePage.isFloating() || mPlacePage.isDocked())
      mDetailsFrame.setPadding(mDetailsFrame.getPaddingLeft(), mDetailsFrame.getPaddingTop(), mDetailsFrame
                                   .getPaddingRight(),
                               UiUtils.dimen(R.dimen.place_page_buttons_height));

    if (mPlacePage.isDocked())
    {
      ViewGroup.LayoutParams lp = mDetailsFrame.getLayoutParams();
      lp.height = ViewGroup.LayoutParams.MATCH_PARENT;
      mDetailsFrame.setLayoutParams(lp);
    }

    initialVisibility();
  }

  @Override
  public void onScroll(int left, int top)
  {
    mCurrentScrollY = top;
  }

  @Override
  public void onScrollEnd()
  {
  }

  protected void initialVisibility()
  {
    UiUtils.invisible(mPlacePage, mPreview, mDetailsFrame, mBookmarkDetails);
  }

  public State getState()
  {
    return mState;
  }

  void setState(final State state, @MapObject.MapObjectType final int type)
  {
    mPlacePage.post(new Runnable()
    {
      @Override
      public void run()
      {
        onStateChanged(mState, state, type);
        mState = state;
      }
    });
  }

  protected abstract void onStateChanged(State currentState, State newState, int type);

  void setOnVisibilityChangedListener(OnVisibilityChangedListener listener)
  {
    mVisibilityListener = listener;
  }

  void setOnProgressListener(@Nullable OnAnimationListener listener)
  {
    mProgressListener = listener;
  }

  protected abstract boolean onInterceptTouchEvent(MotionEvent event);

  protected boolean onTouchEvent(@NonNull MotionEvent event)
  {
    return mGestureDetector.onTouchEvent(event);
  }

  void notifyVisibilityListener(boolean previewShown, boolean ppShown)
  {
    if (mVisibilityListener != null)
    {
      mVisibilityListener.onPreviewVisibilityChanged(previewShown);
      mVisibilityListener.onPlacePageVisibilityChanged(ppShown);
    }
  }

  void notifyProgress(float translationX, float translationY)
  {
    if (mProgressListener == null)
      return;

    mProgressListener.onProgress(translationX, translationY);
  }

  void startDefaultAnimator(ValueAnimator animator)
  {
    startDefaultAnimator(animator, new AccelerateInterpolator());
  }

  void startDefaultAnimator(ValueAnimator animator, Interpolator interpolator)
  {
    animator.setDuration(DURATION);
    animator.setInterpolator(interpolator == null ? INTERPOLATOR : interpolator);
    animator.start();
  }
}
