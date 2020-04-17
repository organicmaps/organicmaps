package com.mapswithme.maps.widget.menu;

import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.View;

import androidx.annotation.NonNull;
import com.google.android.material.bottomsheet.BottomSheetBehavior;
import com.trafi.anchorbottomsheetbehavior.AnchorBottomSheetBehavior;

public class BottomSheetMenuGestureListener extends GestureDetector.SimpleOnGestureListener
{
  @NonNull
  private final BottomSheetBehavior<View> mBottomSheetBehavior;

  public BottomSheetMenuGestureListener(@NonNull BottomSheetBehavior<View> bottomSheetBehavior)
  {
    mBottomSheetBehavior = bottomSheetBehavior;
  }

  @Override
  public boolean onSingleTapConfirmed(MotionEvent e)
  {
    @AnchorBottomSheetBehavior.State
    int state = mBottomSheetBehavior.getState();
    if (!BottomSheetMenuUtils.isHiddenState(state) && !BottomSheetMenuUtils.isDraggingState(state)
        && !BottomSheetMenuUtils.isSettlingState(state))
    {
      mBottomSheetBehavior.setState(BottomSheetBehavior.STATE_HIDDEN);
      return true;
    }

    return false;
  }
}
