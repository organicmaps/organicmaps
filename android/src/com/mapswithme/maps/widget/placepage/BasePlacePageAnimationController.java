package com.mapswithme.maps.widget.placepage;

import android.support.annotation.NonNull;
import android.support.v4.view.GestureDetectorCompat;
import android.view.MotionEvent;
import android.view.ViewConfiguration;
import android.view.ViewGroup;
import android.widget.ScrollView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.maps.widget.placepage.PlacePageView.State;

/**
 * Class is responsible for animations of PP(place page) and PPP(place page preview).
 */
public abstract class BasePlacePageAnimationController
{
  protected static final int SHORT_ANIM_DURATION = 200;
  protected static final int LONG_ANIM_DURATION = 400;

  protected State mState = State.HIDDEN;

  protected PlacePageView mPlacePage;
  protected ViewGroup mPreview;
  protected ScrollView mDetails;
  protected ViewGroup mBookmarkDetails;
  protected ViewGroup mButtons;
  // Gestures
  protected GestureDetectorCompat mGestureDetector;
  protected boolean mIsGestureHandled;
  protected float mDownCoord;
  protected float mTouchSlop;
  // Visibility
  // TODO consider removal
  protected boolean mIsPreviewVisible;
  protected boolean mIsPlacePageVisible;
  protected OnVisibilityChangedListener mVisibilityChangedListener;

  public interface OnVisibilityChangedListener
  {
    void onPreviewVisibilityChanged(boolean isVisible);

    void onPlacePageVisibilityChanged(boolean isVisible);
  }

  abstract void initGestureDetector();

  public BasePlacePageAnimationController(@NonNull PlacePageView placePage)
  {
    mPlacePage = placePage;
    mPreview = (ViewGroup) placePage.findViewById(R.id.pp__preview);
    mDetails = (ScrollView) placePage.findViewById(R.id.pp__details);
    mBookmarkDetails = (ViewGroup) mDetails.findViewById(R.id.rl__bookmark_details);
    mButtons = (ViewGroup) placePage.findViewById(R.id.pp__buttons);
    initGestureDetector();

    mTouchSlop = ViewConfiguration.get(mPlacePage.getContext()).getScaledTouchSlop();
  }

  public void setState(State state, MapObject.MapObjectType type)
  {
    State newState;
    if (type == MapObject.MapObjectType.BOOKMARK && state == State.DETAILS)
      newState = State.BOOKMARK;
    else
      newState = state;

    if (newState != mState)
    {
      animateStateChange(mState, newState);
      mState = newState;
    }
  }

  abstract void animateStateChange(State currentState, State newState);

  public State getState()
  {
    return mState;
  }

  public void setOnVisibilityChangedListener(OnVisibilityChangedListener listener)
  {
    mVisibilityChangedListener = listener;
  }

  abstract boolean onInterceptTouchEvent(MotionEvent event);

  protected boolean onTouchEvent(@NonNull MotionEvent event)
  {
    return mGestureDetector.onTouchEvent(event);
  }

  protected void notifyVisibilityListener()
  {
    if (mVisibilityChangedListener != null)
    {
      mVisibilityChangedListener.onPreviewVisibilityChanged(mIsPreviewVisible);
      mVisibilityChangedListener.onPlacePageVisibilityChanged(mIsPlacePageVisible);
    }
  }
}
