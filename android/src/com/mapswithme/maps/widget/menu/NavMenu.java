package com.mapswithme.maps.widget.menu;

import android.animation.Animator;
import android.animation.ValueAnimator;
import android.graphics.drawable.Drawable;
import android.view.View;
import android.widget.Button;
import android.widget.ImageView;

import androidx.annotation.IntegerRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.mapswithme.maps.Framework;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.maplayer.traffic.TrafficManager;
import com.mapswithme.maps.sound.TtsPlayer;
import com.mapswithme.maps.widget.RotateDrawable;
import com.mapswithme.util.Graphics;
import com.mapswithme.util.UiUtils;

public class NavMenu extends BaseMenu
{
  @IntegerRes
  private final int mAnimationDuration;
  private final RotateDrawable mToggleImage;
  @NonNull
  private final ImageView mTts;
  @NonNull
  private final ImageView mTraffic;

  final View mContentFrame;

  int mContentHeight;

  boolean mLayoutMeasured;

  private boolean mIsOpen;

  private boolean mAnimating;

  public enum Item implements BaseMenu.Item
  {
    TOGGLE(R.id.toggle),
    TTS_VOLUME(R.id.tts_volume),
    STOP(R.id.stop),
    SETTINGS(R.id.settings),
    TRAFFIC(R.id.traffic);

    private final int mViewId;

    Item(int viewId)
    {
      mViewId = viewId;
    }

    @Override
    public int getViewId()
    {
      return mViewId;
    }
  }

  private class AnimationListener extends UiUtils.SimpleAnimatorListener
  {
    @Override
    public void onAnimationStart(android.animation.Animator animation)
    {
      mAnimating = true;
    }

    @Override
    public void onAnimationEnd(android.animation.Animator animation)
    {
      mAnimating = false;
    }
  }

  public NavMenu(View frame, ItemClickListener<Item> listener)
  {
    super(frame, listener);
    mAnimationDuration = MwmApplication.from(frame.getContext())
                                       .getResources().getInteger(R.integer.anim_menu);
    mContentFrame = mFrame.findViewById(R.id.content_frame);
    mToggleImage = new RotateDrawable(Graphics.tint(mFrame.getContext(), R.drawable.ic_menu_close, R.attr.iconTintLight));
    ImageView toggle = (ImageView) mLineFrame.findViewById(R.id.toggle);
    toggle.setImageDrawable(mToggleImage);

    setToggleState(false, false);

    mapItem(Item.TOGGLE, mLineFrame);
    Button stop = (Button) mapItem(Item.STOP, mFrame);
    UiUtils.updateRedButton(stop);

    mapItem(Item.SETTINGS, mFrame);

    mTts = (ImageView) mapItem(Item.TTS_VOLUME, mFrame);
    mTraffic = (ImageView) mapItem(Item.TRAFFIC, mFrame);
  }

  @Override
  public void onResume(@Nullable Runnable procAfterMeasurement)
  {
    measureContent(procAfterMeasurement);
    refresh();
  }

  public boolean isOpen()
  {
    return mIsOpen;
  }

  public boolean isAnimating()
  {
    return mAnimating;
  }

  public boolean open(boolean animate)
  {
    if ((animate && mAnimating) || isOpen())
      return false;

    mIsOpen = true;

    UiUtils.show(mContentFrame);
    adjustTransparency();
    updateMarker();

    setToggleState(mIsOpen, animate);
    if (!animate)
      return true;

    mFrame.setTranslationY(mContentHeight);
    mFrame.animate()
          .setDuration(mAnimationDuration)
          .translationY(0.0f)
          .setListener(new AnimationListener())
          .start();

    return true;
  }

  public boolean close(boolean animate, @Nullable final Runnable onCloseListener)
  {
    if (mAnimating || !isOpen())
    {
      if (onCloseListener != null)
        onCloseListener.run();

      return false;
    }

    mIsOpen = false;
    setToggleState(mIsOpen, animate);

    if (!animate)
    {
      UiUtils.hide(mContentFrame);
      adjustTransparency();
      updateMarker();

      if (onCloseListener != null)
        onCloseListener.run();

      return true;
    }

    mFrame.animate()
          .setDuration(mAnimationDuration)
          .translationY(mContentHeight)
          .setListener(new AnimationListener()
          {
            @Override
            public void onAnimationEnd(Animator animation)
            {
              super.onAnimationEnd(animation);
              mFrame.setTranslationY(0.0f);
              UiUtils.hide(mContentFrame);
              adjustTransparency();
              updateMarker();

              if (onCloseListener != null)
                onCloseListener.run();
            }
          }).start();

    return true;
  }

  public void toggle(boolean animate)
  {
    if (mAnimating)
      return;

    boolean show = !isOpen();

    if (show)
      open(animate);
    else
      close(animate);
  }


  void measureContent(@Nullable final Runnable procAfterMeasurement)
  {
    if (mLayoutMeasured)
      return;

    UiUtils.measureView(mContentFrame, new UiUtils.OnViewMeasuredListener()
    {
      @Override
      public void onViewMeasured(int width, int height)
      {
        if (height != 0)
        {
          mContentHeight = height;
          mLayoutMeasured = true;

          UiUtils.hide(mContentFrame);
        }
        afterLayoutMeasured(procAfterMeasurement);
      }
    });
  }

  public void refresh()
  {
    refreshTts();
    refreshTraffic();
  }

  public void refreshTts()
  {
    mTts.setImageDrawable(TtsPlayer.isEnabled() ? Graphics.tint(mFrame.getContext(), R.drawable.ic_voice_on,
                                                                R.attr.colorAccent)
                                                : Graphics.tint(mFrame.getContext(), R.drawable.ic_voice_off));
  }

  public void refreshTraffic()
  {
    Drawable onIcon = Graphics.tint(mFrame.getContext(), R.drawable.ic_setting_traffic_on,
                                    R.attr.colorAccent);
    Drawable offIcon = Graphics.tint(mFrame.getContext(), R.drawable.ic_setting_traffic_off);
    mTraffic.setImageDrawable(TrafficManager.INSTANCE.isEnabled() ? onIcon : offIcon);
  }

  @Override
  protected void setToggleState(boolean open, boolean animate)
  {
    final float to = open ? -90.0f : 90.0f;
    if (!animate)
    {
      mToggleImage.setAngle(to);
      return;
    }

    final float from = -to;
    ValueAnimator animator = ValueAnimator.ofFloat(from, to);
    animator.addUpdateListener(new ValueAnimator.AnimatorUpdateListener()
    {
      @Override
      public void onAnimationUpdate(ValueAnimator animation)
      {
        mToggleImage.setAngle((float) animation.getAnimatedValue());
      }
    });

    animator.setDuration(mAnimationDuration);
    animator.start();
  }

  @Override
  protected int getHeightResId()
  {
    return R.dimen.nav_menu_height;
  }

  @Override
  protected void adjustTransparency() {}

  @Override
  public void show(boolean show)
  {
    super.show(show);
    measureContent(null);
    @Framework.RouterType
    int routerType = Framework.nativeGetRouter();
    UiUtils.showIf(show, mTts);
    UiUtils.showIf(show && routerType == Framework.ROUTER_TYPE_VEHICLE, mTraffic);
  }
}
