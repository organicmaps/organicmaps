package com.mapswithme.maps.widget.menu;

import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.TransitionDrawable;
import android.support.annotation.DimenRes;
import android.view.View;
import android.widget.ImageView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.widget.RotateByAlphaDrawable;
import com.mapswithme.maps.widget.TrackedTransitionDrawable;
import com.mapswithme.util.UiUtils;

class MenuToggle
{
  private final ImageView mButton;
  private final boolean mAlwaysShow;

  private final TransitionDrawable mOpenImage;
  private final TransitionDrawable mCollapseImage;

  MenuToggle(View frame, @DimenRes int heightRes)
  {
    mButton = (ImageView) frame.findViewById(R.id.toggle);
    mAlwaysShow = (frame.findViewById(R.id.disable_toggle) == null);

    int sz = UiUtils.dimen(heightRes);
    Rect bounds = new Rect(0, 0, sz, sz);

    mOpenImage = new TrackedTransitionDrawable(new Drawable[] { new RotateByAlphaDrawable(frame.getContext(), R.drawable.ic_menu_open, R.attr.iconTint, false)
                                                                    .setInnerBounds(bounds),
                                                                new RotateByAlphaDrawable(frame.getContext(), R.drawable.ic_menu_close, R.attr.iconTintLight, true)
                                                                    .setInnerBounds(bounds)
                                                                    .setBaseAngle(-90) });
    mCollapseImage = new TrackedTransitionDrawable(new Drawable[] { new RotateByAlphaDrawable(frame.getContext(), R.drawable.ic_menu_open, R.attr.iconTint, false)
                                                                        .setInnerBounds(bounds),
                                                                    new RotateByAlphaDrawable(frame.getContext(), R.drawable.ic_menu_close, R.attr.iconTintLight, true)
                                                                        .setInnerBounds(bounds) });
    mOpenImage.setCrossFadeEnabled(true);
    mCollapseImage.setCrossFadeEnabled(true);
  }

  private void transitImage(TransitionDrawable image, boolean forward, boolean animate)
  {
    if (!UiUtils.isVisible(mButton))
      animate = false;

    mButton.setImageDrawable(image);

    if (forward)
      image.startTransition(animate ? BaseMenu.ANIMATION_DURATION : 0);
    else
      image.reverseTransition(animate ? BaseMenu.ANIMATION_DURATION : 0);

    if (!animate)
      image.getDrawable(forward ? 1 : 0).setAlpha(0xFF);
  }

  void show(boolean show)
  {
    UiUtils.showIf(mAlwaysShow || show, mButton);
  }

  void setOpen(boolean open, boolean animate)
  {
    transitImage(mOpenImage, open, animate);
  }

  void setCollapsed(boolean collapse, boolean animate)
  {
    transitImage(mCollapseImage, collapse, animate);
  }
}