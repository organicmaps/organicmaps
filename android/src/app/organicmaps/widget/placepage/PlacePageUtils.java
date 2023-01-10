package app.organicmaps.widget.placepage;

import android.graphics.Rect;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;

import com.google.android.material.bottomsheet.BottomSheetBehavior;
import app.organicmaps.Framework;
import app.organicmaps.R;

class PlacePageUtils
{
  static void moveViewportUp(@NonNull View placePageView, int viewportMinHeight)
  {
    placePageView.post(() -> {
      View coordinatorLayout = (ViewGroup) placePageView.getParent();
      int viewPortWidth = coordinatorLayout.getWidth();
      int viewPortHeight = coordinatorLayout.getHeight();
      Rect sheetRect = new Rect();
      placePageView.getGlobalVisibleRect(sheetRect);
      if (sheetRect.top < viewportMinHeight)
        return;

      if (sheetRect.top >= viewPortHeight)
      {
        Framework.nativeSetVisibleRect(0, 0, viewPortWidth, viewPortHeight);
        return;
      }
      viewPortHeight -= sheetRect.height();
      Framework.nativeSetVisibleRect(0, 0, viewPortWidth, viewPortHeight);
    });
  }

  static void moveViewPortRight(@NonNull View bootomSheet, int viewportMinWidth)
  {
    bootomSheet.post(() -> {
      View coordinatorLayout = (ViewGroup) bootomSheet.getParent();
      int viewPortWidth = coordinatorLayout.getWidth();
      int viewPortHeight = coordinatorLayout.getHeight();
      Rect sheetRect = new Rect();
      bootomSheet.getGlobalVisibleRect(sheetRect);
      if ((viewPortWidth - sheetRect.right) < viewportMinWidth)
      {
        Framework.nativeSetVisibleRect((viewPortWidth - viewportMinWidth), 0, viewPortWidth,
                                       viewPortHeight);
        return;
      }

      if (sheetRect.top >= viewPortHeight)
      {
        Framework.nativeSetVisibleRect(0, 0, viewPortWidth, viewPortHeight);
        return;
      }

      Framework.nativeSetVisibleRect(sheetRect.right, 0, viewPortWidth, viewPortHeight);
    });
  }

  static boolean isSettlingState(@BottomSheetBehavior.State int state)
  {
    return state == BottomSheetBehavior.STATE_SETTLING;
  }

  static boolean isDraggingState(@BottomSheetBehavior.State int state)
  {
    return state == BottomSheetBehavior.STATE_DRAGGING;
  }

  static boolean isCollapsedState(@BottomSheetBehavior.State int state)
  {
    return state == BottomSheetBehavior.STATE_COLLAPSED;
  }

  static boolean isHalfExpandedState(@BottomSheetBehavior.State int state)
  {
    return state == BottomSheetBehavior.STATE_HALF_EXPANDED;
  }


  static boolean isExpandedState(@BottomSheetBehavior.State int state)
  {
    return state == BottomSheetBehavior.STATE_EXPANDED;
  }

  static boolean isHiddenState(@BottomSheetBehavior.State int state)
  {
    return state == BottomSheetBehavior.STATE_HIDDEN;
  }

  @NonNull
  static String toString(@BottomSheetBehavior.State int state)
  {
    switch (state)
    {
      case BottomSheetBehavior.STATE_EXPANDED:
        return "EXPANDED";
      case BottomSheetBehavior.STATE_COLLAPSED:
        return "COLLAPSED";
      case BottomSheetBehavior.STATE_HALF_EXPANDED:
        return "HALF_EXPANDED";
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
