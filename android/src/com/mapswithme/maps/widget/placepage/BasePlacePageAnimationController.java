package com.mapswithme.maps.widget.placepage;

import android.support.annotation.NonNull;
import android.support.v4.view.GestureDetectorCompat;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewConfiguration;

import com.mapswithme.maps.R;

/**
 * Class is responsible for animations of PP(place page) and PPP(place page preview).
 */
public abstract class BasePlacePageAnimationController
{
  protected static final int SHORT_ANIM_DURATION = 200;
  protected static final int LONG_ANIM_DURATION = 400;

  protected PlacePageView mPlacePage;
  protected View mPreview;
  protected View mPlacePageDetails;
  // Gestures
  protected GestureDetectorCompat mGestureDetector;
  protected boolean mIsGestureHandled;
  protected float mDownY;
  protected float mTouchSlop;
  // Visibility
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
    mPreview = placePage.findViewById(R.id.preview);
    mPlacePageDetails = placePage.findViewById(R.id.place_page);
    initGestureDetector();

    mTouchSlop = ViewConfiguration.get(mPlacePage.getContext()).getScaledTouchSlop();
  }

  public void setOnVisibilityChangedListener(OnVisibilityChangedListener listener)
  {
    mVisibilityChangedListener = listener;
  }

  abstract boolean onInterceptTouchEvent(MotionEvent event);

  protected void onTouchEvent(@NonNull MotionEvent event)
  {
    mPlacePage.requestDisallowInterceptTouchEvent(false);
    mGestureDetector.onTouchEvent(event);
  }
}
