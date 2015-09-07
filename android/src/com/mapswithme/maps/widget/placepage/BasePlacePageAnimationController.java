package com.mapswithme.maps.widget.placepage;

import android.support.annotation.NonNull;
import android.support.v4.view.GestureDetectorCompat;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewConfiguration;
import android.view.ViewGroup;
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

  protected State mState = State.HIDDEN;

  protected PlacePageView mPlacePage;
  protected ViewGroup mPreview;
  protected ViewGroup mFrame;
  protected ScrollView mDetails;
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
    mFrame = (ViewGroup) placePage.findViewById(R.id.pp__details_frame);
    mDetails = (ScrollView) placePage.findViewById(R.id.pp__details);
    mDetailsContent = mDetails.getChildAt(0);
    mBookmarkDetails = (ViewGroup) mFrame.findViewById(R.id.rl__bookmark_details);
    mButtons = (ViewGroup) placePage.findViewById(R.id.pp__buttons);
    initGestureDetector();

    mTouchSlop = ViewConfiguration.get(mPlacePage.getContext()).getScaledTouchSlop();

    if (mPlacePage.isFloating() || mPlacePage.isDocked())
      mFrame.setPadding(mFrame.getPaddingLeft(), mFrame.getPaddingTop(), mFrame.getPaddingRight(),
                        UiUtils.dimen(R.dimen.place_page_buttons_height));

    if (mPlacePage.isDocked())
    {
      ViewGroup.LayoutParams lp = mFrame.getLayoutParams();
      lp.height = ViewGroup.LayoutParams.MATCH_PARENT;
      mFrame.setLayoutParams(lp);
    }
  }

  public void setState(State state, MapObject.MapObjectType type)
  {
    State newState = state;
    if (type == MapObject.MapObjectType.BOOKMARK && state == State.DETAILS)
      newState = State.BOOKMARK;

    if (newState != mState)
    {
      onStateChanged(mState, newState);
      mState = newState;
    }
  }

  protected void initialHide()
  {
    UiUtils.invisible(mPlacePage);
  }

  protected abstract void onStateChanged(State currentState, State newState);

  public State getState()
  {
    return mState;
  }

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
}
