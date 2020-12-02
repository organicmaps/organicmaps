package com.mapswithme.maps.widget.menu;

import android.content.Context;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.TransitionDrawable;
import androidx.annotation.DimenRes;
import androidx.annotation.DrawableRes;
import android.view.View;
import android.widget.ImageView;

import androidx.annotation.IntegerRes;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.widget.RotateByAlphaDrawable;
import com.mapswithme.maps.widget.TrackedTransitionDrawable;
import com.mapswithme.util.UiUtils;

class MenuToggle
{
  @IntegerRes
  private final int mAnimationDuration;
  private final ImageView mButton;
  private final TransitionDrawable mOpenImage;
  private final TransitionDrawable mCollapseImage;

  MenuToggle(View frame, @DimenRes int heightRes)
  {
    this(frame, heightRes, R.drawable.ic_menu_open, R.drawable.ic_menu_close);
  }

  private MenuToggle(View frame, @DimenRes int heightRes, @DrawableRes int src, @DrawableRes int dst)
  {
    mButton = frame.findViewById(R.id.toggle);
    Context context = frame.getContext();
    mAnimationDuration = context.getResources().getInteger(R.integer.anim_menu);
    int sz = UiUtils.dimen(context, heightRes);
    Rect bounds = new Rect(0, 0, sz, sz);

    mOpenImage = new TrackedTransitionDrawable(new Drawable[]{
        new RotateByAlphaDrawable(context, src, R.attr.iconTint, false)
            .setInnerBounds(bounds),
        new RotateByAlphaDrawable(context, dst, R.attr.iconTintLight, true)
            .setInnerBounds(bounds)
            .setBaseAngle(-90)});
    mCollapseImage = new TrackedTransitionDrawable(new Drawable[]{
        new RotateByAlphaDrawable(context, src, R.attr.iconTint, false)
            .setInnerBounds(bounds),
        new RotateByAlphaDrawable(context, dst, R.attr.iconTintLight, true)
            .setInnerBounds(bounds)});
    mOpenImage.setCrossFadeEnabled(true);
    mCollapseImage.setCrossFadeEnabled(true);
  }

  private void transitImage(TransitionDrawable image, boolean forward, boolean animate)
  {
    if (!UiUtils.isVisible(mButton))
      animate = false;

    mButton.setImageDrawable(image);

    if (forward)
      image.startTransition(animate ? mAnimationDuration : 0);
    else
      image.reverseTransition(animate ? mAnimationDuration : 0);

    if (!animate)
      image.getDrawable(forward ? 1 : 0).setAlpha(0xFF);
  }

  void show(boolean show)
  {
    UiUtils.showIf(show, mButton);
  }

  void hide()
  {
    UiUtils.hide(mButton);
  }

  void setOpen(boolean open, boolean animate)
  {
    transitImage(mOpenImage, open, animate);
  }
}
