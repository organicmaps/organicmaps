package com.mapswithme.maps.widget.menu;

import android.annotation.SuppressLint;
import android.view.View;

import androidx.annotation.IdRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.view.GestureDetectorCompat;
import com.google.android.material.bottomsheet.BottomSheetBehavior;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

import java.util.Objects;

public class BottomSheetMenuController implements MenuController
{
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);
  private static final String TAG = BottomSheetMenuController.class.getSimpleName();
  @SuppressWarnings("NullableProblems")
  @NonNull
  private BottomSheetBehavior<View> mSheetBehavior;
  @IdRes
  private final int mSheetResId;
  @NonNull
  private final MenuRenderer mMenuRenderer;
  @Nullable
  private MenuStateObserver mStateObserver;
  private final BottomSheetBehavior.BottomSheetCallback mSheetCallback
      = new BottomSheetBehavior.BottomSheetCallback()
  {
    @Override
    public void onStateChanged(@NonNull View view, int state)
    {
      LOGGER.d(TAG, "State change, new = " + BottomSheetMenuUtils.toString(state));
      if (BottomSheetMenuUtils.isSettlingState(state) || BottomSheetMenuUtils.isDraggingState(state))
        return;

      if (BottomSheetMenuUtils.isHiddenState(state))
      {
        mMenuRenderer.onHide();
        if (mStateObserver != null)
          mStateObserver.onMenuClosed();
        return;
      }

      if (mStateObserver != null)
        mStateObserver.onMenuOpen();
    }

    @Override
    public void onSlide(@NonNull View view, float v)
    {
      // Do nothing by default.
    }
  };

  BottomSheetMenuController(int sheetResId, @NonNull MenuRenderer menuRenderer,
                            @Nullable MenuStateObserver stateObserver)
  {
    mSheetResId = sheetResId;
    mMenuRenderer = menuRenderer;
    mStateObserver = stateObserver;
  }

  @Override
  public void open()
  {
    mMenuRenderer.render();
    mSheetBehavior.setState(BottomSheetBehavior.STATE_EXPANDED);
  }

  @Override
  public void close()
  {
    mSheetBehavior.setState(BottomSheetBehavior.STATE_HIDDEN);
  }

  @Override
  public boolean isClosed()
  {
    int state = mSheetBehavior.getState();
    return BottomSheetMenuUtils.isHiddenState(state)
           || (mSheetBehavior.getSkipCollapsed() && BottomSheetMenuUtils.isCollapsedState(state));
  }

  @SuppressLint("ClickableViewAccessibility")
  @Override
  public void initialize(@Nullable View view)
  {
    Objects.requireNonNull(view);
    View sheet = view.findViewById(mSheetResId);
    Objects.requireNonNull(sheet);
    mSheetBehavior = BottomSheetBehavior.from(sheet);
    mSheetBehavior.setBottomSheetCallback(mSheetCallback);
    mSheetBehavior.setState(BottomSheetBehavior.STATE_HIDDEN);
    GestureDetectorCompat gestureDetector = new GestureDetectorCompat(
        view.getContext(), new BottomSheetMenuGestureListener(mSheetBehavior));
    sheet.setOnTouchListener((v, event) -> gestureDetector.onTouchEvent(event));
    mMenuRenderer.initialize(sheet);
  }

  @Override
  public void destroy()
  {
    mMenuRenderer.destroy();
  }
}
