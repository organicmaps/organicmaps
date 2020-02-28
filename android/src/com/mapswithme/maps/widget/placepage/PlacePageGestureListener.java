package com.mapswithme.maps.widget.placepage;

import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.View;

import androidx.annotation.NonNull;
import com.trafi.anchorbottomsheetbehavior.AnchorBottomSheetBehavior;

class PlacePageGestureListener extends GestureDetector.SimpleOnGestureListener
{
  @NonNull
  private final AnchorBottomSheetBehavior<View> mBottomSheetBehavior;

  PlacePageGestureListener(@NonNull AnchorBottomSheetBehavior<View> bottomSheetBehavior)
  {
    mBottomSheetBehavior = bottomSheetBehavior;
  }

  @NonNull
  AnchorBottomSheetBehavior<View> getBottomSheetBehavior()
  {
    return mBottomSheetBehavior;
  }

  @Override
  public boolean onSingleTapConfirmed(MotionEvent e)
  {
    @AnchorBottomSheetBehavior.State
    int state = mBottomSheetBehavior.getState();
    if (PlacePageUtils.isCollapsedState(state))
    {
      mBottomSheetBehavior.setState(AnchorBottomSheetBehavior.STATE_ANCHORED);
      return true;
    }

    if (PlacePageUtils.isAnchoredState(state) || PlacePageUtils.isExpandedState(state))
    {
      mBottomSheetBehavior.setState(AnchorBottomSheetBehavior.STATE_COLLAPSED);
      return true;
    }

    return false;
  }
}
