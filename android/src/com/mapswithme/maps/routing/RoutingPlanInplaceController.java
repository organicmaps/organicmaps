package com.mapswithme.maps.routing;

import android.animation.Animator;
import android.animation.ValueAnimator;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.widget.ImageView;
import android.widget.TextView;

import com.mapswithme.maps.MwmActivity;
import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.util.UiUtils;

public class RoutingPlanInplaceController extends RoutingPlanController
{
  private static final String STATE_OPEN = "slots panel open";

  private Boolean mSlotsRestoredState;

  public RoutingPlanInplaceController(MwmActivity activity)
  {
    super(activity.findViewById(R.id.routing_plan_frame), activity);
  }

  public void show(final boolean show)
  {
    if (show == UiUtils.isVisible(mFrame))
      return;

    if (show)
    {
      final MapObject start = RoutingController.get().getStartPoint();
      final MapObject end = RoutingController.get().getEndPoint();
      boolean open = (mSlotsRestoredState == null
                        ? (!MapObject.isOfType(MapObject.MY_POSITION, start) || end == null)
                        : mSlotsRestoredState);
      showSlots(open, false);
      mSlotsRestoredState = null;
    }

    if (show)
    {
      UiUtils.show(mFrame);
      updatePoints();
    }

    animateFrame(show, new Runnable()
    {
      @Override
      public void run()
      {
        if (!show)
          UiUtils.hide(mFrame);
      }
    });
  }

  public void onSaveState(@NonNull Bundle outState)
  {
    outState.putBoolean(STATE_OPEN, isOpen());
    saveRoutingPanelState(outState);
  }

  public void restoreState(@NonNull Bundle state)
  {
    if (state.containsKey(STATE_OPEN))
      mSlotsRestoredState = state.getBoolean(STATE_OPEN);

    restoreRoutingPanelState(state);
  }

  @Override
  public void showRouteAltitudeChart()
  {
    ImageView altitudeChart = (ImageView) mActivity.findViewById(R.id.altitude_chart);
    TextView altitudeDifference = (TextView) mActivity.findViewById(R.id.altitude_difference);
    showRouteAltitudeChartInternal(altitudeChart, altitudeDifference);
  }

  private void animateFrame(final boolean show, final @Nullable Runnable completion)
  {
    if (!checkFrameHeight())
    {
      mFrame.post(new Runnable()
      {
        @Override
        public void run()
        {
          animateFrame(show, completion);
        }
      });
      return;
    }

    ValueAnimator animator =
        ValueAnimator.ofFloat(show ? -mFrameHeight : 0, show ? 0 : -mFrameHeight);
    animator.addUpdateListener(new ValueAnimator.AnimatorUpdateListener()
    {
      @Override
      public void onAnimationUpdate(ValueAnimator animation)
      {
        mFrame.setTranslationY((Float) animation.getAnimatedValue());
      }
    });
    animator.addListener(new UiUtils.SimpleAnimatorListener()
    {
      @Override
      public void onAnimationEnd(Animator animation)
      {
        if (completion != null)
          completion.run();
      }
    });
    animator.setDuration(ANIM_TOGGLE);
    animator.start();
  }
}
