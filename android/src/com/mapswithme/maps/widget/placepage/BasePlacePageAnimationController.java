package com.mapswithme.maps.widget.placepage;

import android.support.annotation.NonNull;
import android.support.v4.view.GestureDetectorCompat;
import android.view.MotionEvent;
import android.view.ViewConfiguration;
import android.view.ViewGroup;

import com.mapswithme.maps.R;

/**
 * Class is responsible for animations of PP(place page) and PPP(place page preview).
 */
public abstract class BasePlacePageAnimationController
{
  protected static final int SHORT_ANIM_DURATION = 200;
  protected static final int LONG_ANIM_DURATION = 400;

  protected PlacePageView mPlacePage;
  protected ViewGroup mPreview;
  protected ViewGroup mDetails;
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
    public void onPreviewVisibilityChanged(boolean isVisible);

    public void onPlacePageVisibilityChanged(boolean isVisible);
  }

  abstract void initGestureDetector();

  abstract void showPreview(boolean show);

  abstract void showPlacePage(boolean show);

  abstract void hidePlacePage();

  public BasePlacePageAnimationController(@NonNull PlacePageView placePage)
  {
    mPlacePage = placePage;
    mPreview = (ViewGroup) placePage.findViewById(R.id.pp__preview);
    mDetails = (ViewGroup) placePage.findViewById(R.id.pp__details);
    // we don't want to block details scrolling
    mDetails.requestDisallowInterceptTouchEvent(true);
    mButtons = (ViewGroup) placePage.findViewById(R.id.pp__buttons);
    initGestureDetector();

    mTouchSlop = ViewConfiguration.get(mPlacePage.getContext()).getScaledTouchSlop();
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
}
