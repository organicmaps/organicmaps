package com.mapswithme.maps.widget.menu;

import android.graphics.drawable.AnimationDrawable;
import android.graphics.drawable.Drawable;

import androidx.annotation.NonNull;
import android.util.SparseArray;
import android.view.View;
import android.widget.ImageView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.location.LocationState;
import com.mapswithme.maps.routing.RoutingController;
import com.mapswithme.util.Graphics;
import com.mapswithme.util.ThemeUtils;
import com.mapswithme.util.UiUtils;

public class MyPositionButton
{
  private static final int FOLLOW_SHIFT = 1;

  @NonNull
  private final ImageView mButton;
  private static final SparseArray<Drawable> mIcons = new SparseArray<>(); // Location mode -> Button icon

  private int mMode;
  private boolean mVisible;

  private final int mFollowPaddingShift;

  public MyPositionButton(@NonNull View button, @NonNull View.OnClickListener listener)
  {
    mButton = (ImageView) button;
    mVisible = UiUtils.isVisible(mButton);
    mButton.setOnClickListener(listener);
    mIcons.clear();
    mFollowPaddingShift = (int) (FOLLOW_SHIFT * button.getResources().getDisplayMetrics().density);
  }

  @SuppressWarnings("deprecation")
  public void update(int mode)
  {
    mMode = mode;
    Drawable image = mIcons.get(mode);
    if (image == null)
    {
      switch (mode)
      {
        case LocationState.PENDING_POSITION:
          image = mButton.getResources()
                         .getDrawable(ThemeUtils.getResource(mButton.getContext(), R.attr.myPositionButtonAnimation));
          break;

        case LocationState.NOT_FOLLOW_NO_POSITION:
        case LocationState.NOT_FOLLOW:
          image = Graphics.tint(mButton.getContext(), R.drawable.ic_not_follow);
          break;

        case LocationState.FOLLOW:
          image = Graphics.tint(mButton.getContext(), R.drawable.ic_follow, R.attr.colorAccent);
          break;

        case LocationState.FOLLOW_AND_ROTATE:
          image = Graphics.tint(mButton.getContext(), R.drawable.ic_follow_and_rotate, R.attr.colorAccent);
          break;

        default:
          throw new IllegalArgumentException("Invalid button mode: " + mode);
      }

      mIcons.put(mode, image);
    }

    mButton.setImageDrawable(image);
    updatePadding(mode);

    if (image instanceof AnimationDrawable)
      ((AnimationDrawable) image).start();

    UiUtils.visibleIf(!shouldBeHidden(), mButton);
  }

  private void updatePadding(int mode)
  {
    if (mode == LocationState.FOLLOW)
      mButton.setPadding(0, mFollowPaddingShift, mFollowPaddingShift, 0);
    else
      mButton.setPadding(0, 0, 0, 0);
  }

  private boolean shouldBeHidden()
  {
    return (mMode == LocationState.FOLLOW_AND_ROTATE
            && (RoutingController.get().isPlanning()))
           || !mVisible;
  }

  public void show()
  {
    UiUtils.show(mButton);
  }

  public void hide()
  {
    UiUtils.hide(mButton);
  }
}
