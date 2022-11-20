package app.organicmaps.widget.placepage;

import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.View;

import androidx.annotation.NonNull;
import com.google.android.material.bottomsheet.BottomSheetBehavior;

class PlacePageGestureListener extends GestureDetector.SimpleOnGestureListener
{
  @NonNull
  private final BottomSheetBehavior<View> mBottomSheetBehavior;

  PlacePageGestureListener(@NonNull BottomSheetBehavior<View> bottomSheetBehavior)
  {
    mBottomSheetBehavior = bottomSheetBehavior;
  }

  @NonNull
  BottomSheetBehavior<View> getBottomSheetBehavior()
  {
    return mBottomSheetBehavior;
  }

  @Override
  public boolean onSingleTapConfirmed(MotionEvent e)
  {
    @BottomSheetBehavior.State
    int state = mBottomSheetBehavior.getState();
    if (PlacePageUtils.isCollapsedState(state))
    {
      mBottomSheetBehavior.setState(BottomSheetBehavior.STATE_EXPANDED);
      return true;
    }

    if (PlacePageUtils.isExpandedState(state))
    {
      mBottomSheetBehavior.setState(BottomSheetBehavior.STATE_COLLAPSED);
      return true;
    }

    return false;
  }
}
