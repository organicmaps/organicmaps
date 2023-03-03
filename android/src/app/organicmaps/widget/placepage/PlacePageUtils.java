package app.organicmaps.widget.placepage;

import android.content.Context;
import android.view.Menu;
import android.view.View;
import android.widget.PopupMenu;

import androidx.annotation.NonNull;
import app.organicmaps.Framework;
import app.organicmaps.R;
import app.organicmaps.util.Utils;
import app.organicmaps.display.DisplayManager;
import com.google.android.material.bottomsheet.BottomSheetBehavior;

import java.util.List;

public class PlacePageUtils
{
  static void updateMapViewport(@NonNull View parent, int placePageDistanceToTop, int viewportMinHeight)
  {
    parent.post(() -> {
      // Because of the post(), this lambda is called after the car.SurfaceRenderer.onStableAreaChanged() and breaks the visibleRect configuration
      if (DisplayManager.from(parent.getContext()).isCarDisplayUsed())
        return;
      final int screenWidth = parent.getWidth();
      if (placePageDistanceToTop >= viewportMinHeight)
        Framework.nativeSetVisibleRect(0, 0, screenWidth, placePageDistanceToTop);
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

  public static void copyToClipboard(Context context, View frame, String text)
  {
    Utils.copyTextToClipboard(context, text);
    Utils.showSnackbarAbove(frame,
                            frame.getRootView().findViewById(R.id.pp_buttons_layout),
                            context.getString(R.string.copied_to_clipboard, text));
  }

  public static void showCopyPopup(Context context, View popupAnchor, View frame, List<String> items)
  {
    final PopupMenu popup = new PopupMenu(context, popupAnchor);
    final Menu menu = popup.getMenu();
    final String copyText = context.getResources().getString(android.R.string.copy);

    for (int i = 0; i < items.size(); i++)
      menu.add(Menu.NONE, i, i, String.format("%s %s", copyText, items.get(i)));

    popup.setOnMenuItemClickListener(item -> {
      final String text = items.get(item.getItemId());
      copyToClipboard(context, frame, text);
      return true;
    });
    popup.show();
  }
}
