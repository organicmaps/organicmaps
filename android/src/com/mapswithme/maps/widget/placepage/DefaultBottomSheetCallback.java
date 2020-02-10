package com.mapswithme.maps.widget.placepage;

import android.view.View;

import androidx.annotation.NonNull;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;
import com.trafi.anchorbottomsheetbehavior.AnchorBottomSheetBehavior;

import static com.mapswithme.maps.widget.placepage.PlacePageUtils.isAnchoredState;
import static com.mapswithme.maps.widget.placepage.PlacePageUtils.isExpandedState;

public class DefaultBottomSheetCallback extends AnchorBottomSheetBehavior.BottomSheetCallback
{
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);
  private static final String TAG = DefaultBottomSheetCallback.class.getSimpleName();
  @NonNull
  private final BottomSheetChangedListener mSheetChangedListener;

  DefaultBottomSheetCallback(@NonNull BottomSheetChangedListener sheetChangedListener)
  {
    mSheetChangedListener = sheetChangedListener;
  }

  @Override
  public void onStateChanged(@NonNull View bottomSheet, int oldState, int newState)
  {
    LOGGER.d(TAG, "State change, new = " + PlacePageUtils.toString(newState)
                  + " old = " + PlacePageUtils.toString(oldState));
    if (PlacePageUtils.isSettlingState(newState) || PlacePageUtils.isDraggingState(newState))
    {
      return;
    }

    if (PlacePageUtils.isHiddenState(newState))
    {
      mSheetChangedListener.onSheetHidden();
      return;
    }

    mSheetChangedListener.onSheetDirectionIconChange();

    if (isAnchoredState(newState) || isExpandedState(newState))
    {
      mSheetChangedListener.onSheetDetailsOpened();
      return;
    }

    mSheetChangedListener.onSheetCollapsed();
  }

  @Override
  public void onSlide(@NonNull View bottomSheet, float slideOffset)
  {
    mSheetChangedListener.onSheetSliding(bottomSheet.getTop());

    if (slideOffset < 0)
      return;

    mSheetChangedListener.onSheetSlideFinish();
  }
}
