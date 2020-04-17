package com.mapswithme.maps.widget.menu;

import androidx.annotation.NonNull;
import com.google.android.material.bottomsheet.BottomSheetBehavior;

class BottomSheetMenuUtils
{
  static boolean isSettlingState(int state)
  {
    return state == BottomSheetBehavior.STATE_SETTLING;
  }

  static boolean isDraggingState(int state)
  {
    return state == BottomSheetBehavior.STATE_DRAGGING;
  }

  static boolean isCollapsedState(int state)
  {
    return state == BottomSheetBehavior.STATE_COLLAPSED;
  }

  static boolean isExpandedState(int state)
  {
    return state == BottomSheetBehavior.STATE_EXPANDED;
  }

  static boolean isHiddenState(int state)
  {
    return state == BottomSheetBehavior.STATE_HIDDEN;
  }

  @NonNull
  static String toString(int state)
  {
    switch (state)
    {
      case BottomSheetBehavior.STATE_EXPANDED:
        return "EXPANDED";
      case BottomSheetBehavior.STATE_COLLAPSED:
        return "COLLAPSED";
      case BottomSheetBehavior.STATE_DRAGGING:
        return "DRAGGING";
      case BottomSheetBehavior.STATE_SETTLING:
        return "SETTLING";
      case BottomSheetBehavior.STATE_HIDDEN:
        return "HIDDEN";
      default:
        throw new AssertionError("Unsupported state detected: " + state);
    }
  }
}
