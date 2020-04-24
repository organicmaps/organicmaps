package com.mapswithme.maps.widget.placepage;

import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;

import androidx.annotation.DrawableRes;
import androidx.annotation.IdRes;
import androidx.annotation.NonNull;
import com.mapswithme.maps.Framework;
import com.mapswithme.maps.R;
import com.mapswithme.util.Graphics;
import com.mapswithme.util.UiUtils;
import com.trafi.anchorbottomsheetbehavior.AnchorBottomSheetBehavior;

class PlacePageUtils
{
  static final String EXTRA_PLACE_PAGE_DATA = "extra_place_page_data";

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

  static void setPullDrawable(@NonNull AnchorBottomSheetBehavior behavior, @NonNull View bottomSheet,
                              @IdRes int pullDrawableId)
  {
    final ImageView img = bottomSheet.findViewById(pullDrawableId);
    if (img == null)
      return;

    @AnchorBottomSheetBehavior.State
    int state = behavior.getState();
    @DrawableRes
    int drawableId = UiUtils.NO_ID;
    if (PlacePageUtils.isCollapsedState(state))
      drawableId = R.drawable.ic_disclosure_up;
    else if (PlacePageUtils.isAnchoredState(state) || PlacePageUtils.isExpandedState(state))
      drawableId = R.drawable.ic_disclosure_down;

    if (drawableId == UiUtils.NO_ID)
      return;

    Drawable drawable = Graphics.tint(bottomSheet.getContext(), drawableId,
                                      R.attr.bannerButtonBackgroundColor);
    img.setImageDrawable(drawable);
  }

  static boolean isSettlingState(@AnchorBottomSheetBehavior.State int state)
  {
    return state == AnchorBottomSheetBehavior.STATE_SETTLING;
  }

  static boolean isDraggingState(@AnchorBottomSheetBehavior.State int state)
  {
    return state == AnchorBottomSheetBehavior.STATE_DRAGGING;
  }

  static boolean isCollapsedState(@AnchorBottomSheetBehavior.State int state)
  {
    return state == AnchorBottomSheetBehavior.STATE_COLLAPSED;
  }

  static boolean isAnchoredState(@AnchorBottomSheetBehavior.State int state)
  {
    return state == AnchorBottomSheetBehavior.STATE_ANCHORED;
  }

  static boolean isExpandedState(@AnchorBottomSheetBehavior.State int state)
  {
    return state == AnchorBottomSheetBehavior.STATE_EXPANDED;
  }

  static boolean isHiddenState(@AnchorBottomSheetBehavior.State int state)
  {
    return state == AnchorBottomSheetBehavior.STATE_HIDDEN;
  }

  @NonNull
  static String toString(@AnchorBottomSheetBehavior.State int state)
  {
    switch (state)
    {
      case AnchorBottomSheetBehavior.STATE_EXPANDED:
        return "EXPANDED";
      case AnchorBottomSheetBehavior.STATE_COLLAPSED:
        return "COLLAPSED";
      case AnchorBottomSheetBehavior.STATE_ANCHORED:
        return "ANCHORED";
      case AnchorBottomSheetBehavior.STATE_DRAGGING:
        return "DRAGGING";
      case AnchorBottomSheetBehavior.STATE_SETTLING:
        return "SETTLING";
      case AnchorBottomSheetBehavior.STATE_HIDDEN:
        return "HIDDEN";
      default:
        throw new AssertionError("Unsupported state detected: " + state);
    }
  }
}
