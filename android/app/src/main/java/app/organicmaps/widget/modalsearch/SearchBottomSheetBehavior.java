package app.organicmaps.widget.modalsearch;

import android.content.Context;
import android.util.AttributeSet;
import android.view.View;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.lifecycle.Lifecycle;
import androidx.lifecycle.LifecycleEventObserver;

import com.google.android.material.bottomsheet.BottomSheetBehavior;

public class SearchBottomSheetBehavior<V extends View> extends BottomSheetBehavior<V>
{
  private float mHalfExpandedBaseRatio = 0.5f;
  private float mHalfExpandedRatio = mHalfExpandedBaseRatio;
  private final SheetCollapseHelper sheetSlideHelper = new SheetCollapseHelper()
  {
    private static final float COLLAPSE_VELOCITY_THRESHOLD = 0.2f; // offset units per second
    private static final long VELOCITY_TRACKING_TIMEOUT_MS = 150;
    private float mLastSlideOffset = 0f;
    private long mLastSlideTimeMillis = 0;

    public void onSlide(float slideOffset)
    {
      if (getSkipCollapsed() || getState() != STATE_DRAGGING || slideOffset >= getHalfExpandedRatio())
        return;
      long currentTimeMillis = System.currentTimeMillis();
      if (currentTimeMillis - mLastSlideTimeMillis < VELOCITY_TRACKING_TIMEOUT_MS)
      {
        // Calculate velocity (negative means downward movement)
        float timeDelta = (currentTimeMillis - mLastSlideTimeMillis) / 1000f; // convert to seconds
        float offsetDelta = slideOffset - mLastSlideOffset;
        float velocity = offsetDelta / timeDelta;
        if (velocity < -COLLAPSE_VELOCITY_THRESHOLD)
        {
          // detected a fast downward swipe from half-expanded state, so collapse
          setState(BottomSheetBehavior.STATE_COLLAPSED);
        }
      }
      mLastSlideTimeMillis = currentTimeMillis;
      mLastSlideOffset = slideOffset;
    }
  };
  private final BottomSheetCallback easierCollapseCallback = new BottomSheetCallback()
  {
    @Override
    public void onStateChanged(@NonNull View bottomSheet, int newState)
    {
      if (newState == STATE_HALF_EXPANDED && mHalfExpandedRatio != getHalfExpandedRatio())
        setState(STATE_HALF_EXPANDED);
    }

    @Override
    public void onSlide(@NonNull View bottomSheet, float slideOffset)
    {
      sheetSlideHelper.onSlide(slideOffset);
    }
  };

  public SearchBottomSheetBehavior(@NonNull Context context, @Nullable AttributeSet attrs)
  {
    super(context, attrs);
  }

  public static <V extends View> SearchBottomSheetBehavior<V> from(@NonNull V view, Lifecycle lifecycle, float baseHalfExpandedRatio)
  {
    SearchBottomSheetBehavior<V> result = (SearchBottomSheetBehavior<V>) SearchBottomSheetBehavior.from(view);
    lifecycle.addObserver((LifecycleEventObserver) (lifecycleOwner, event) -> {
      switch (event)
      {
      case ON_START -> result.onStart();
      case ON_STOP -> result.onStop();
      }
    });
    result.mHalfExpandedBaseRatio = Math.min(0.999f, baseHalfExpandedRatio); // Limit the ratio to the maximum allowed value of less than 1.0f
    result.mHalfExpandedRatio = result.mHalfExpandedBaseRatio;
    return result;
  }

  @Override
  public void setState(int state)
  {
    if (state == STATE_HALF_EXPANDED)
      mHalfExpandedRatio = getHalfExpandedRatio();
    super.setState(state);
  }

  public void onStart()
  {
    addBottomSheetCallback(easierCollapseCallback);
  }

  private void onStop()
  {
    removeBottomSheetCallback(easierCollapseCallback);
  }

  public void updateUnavailableScreenRatio(float unavailableRatio)
  {
    if (unavailableRatio < 0)
      return;
    setHalfExpandedRatio(Math.min(0.999f, mHalfExpandedBaseRatio + unavailableRatio));
    if (getState() == STATE_HALF_EXPANDED)
      super.setState(BottomSheetBehavior.STATE_HALF_EXPANDED); // intended super call; calling self setState would defeat the purpose of mHalfExpandedRatio
  }

  private interface SheetCollapseHelper
  {
    void onSlide(float slideOffset);
  }
}
