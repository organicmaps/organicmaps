package app.organicmaps.widget.placepage;

import android.view.View;

import androidx.annotation.NonNull;
import com.google.android.material.bottomsheet.BottomSheetBehavior;
import app.organicmaps.util.log.Logger;

import static app.organicmaps.widget.placepage.PlacePageUtils.isExpandedState;

public class DefaultBottomSheetCallback extends BottomSheetBehavior.BottomSheetCallback
{
  private static final String TAG = DefaultBottomSheetCallback.class.getSimpleName();
  @NonNull
  private final BottomSheetChangedListener mSheetChangedListener;

  DefaultBottomSheetCallback(@NonNull BottomSheetChangedListener sheetChangedListener)
  {
    mSheetChangedListener = sheetChangedListener;
  }

  @Override
  public void onStateChanged(@NonNull View bottomSheet, int newState)
  {
    Logger.d(TAG, "State change, new = " + PlacePageUtils.toString(newState));
    if (PlacePageUtils.isSettlingState(newState) || PlacePageUtils.isDraggingState(newState))
      return;

    mSheetChangedListener.onSheetSlideFinish();

    if (PlacePageUtils.isHiddenState(newState))
    {
      mSheetChangedListener.onSheetHidden();
      return;
    }

    if (isExpandedState(newState))
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
  }
}
