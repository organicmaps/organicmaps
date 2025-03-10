package app.organicmaps.widget.modalsearch;

import android.view.View;

import androidx.annotation.NonNull;

import com.google.android.material.bottomsheet.BottomSheetBehavior;

import app.organicmaps.Framework;
import app.organicmaps.display.DisplayManager;

public class ModalSearchUtils
{

  static void updateMapViewport(@NonNull View parent, int newSearchDistanceToTop, int viewportMinHeight)
  {
    parent.post(() -> {
      // Because of the post(), this lambda is called after the car.SurfaceRenderer.onStableAreaChanged() and breaks the visibleRect configuration
      if (DisplayManager.from(parent.getContext()).isCarDisplayUsed())
        return;
      final int screenWidth = parent.getWidth();
      if (newSearchDistanceToTop >= viewportMinHeight)
        Framework.nativeSetVisibleRect(0, 0, screenWidth, newSearchDistanceToTop);
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

  static boolean isHiddenState(@BottomSheetBehavior.State int state)
  {
    return state == BottomSheetBehavior.STATE_HIDDEN;
  }
}
