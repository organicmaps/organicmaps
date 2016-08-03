package com.mapswithme.maps.widget.menu;

import android.animation.ValueAnimator;
import android.support.annotation.Nullable;
import android.view.View;
import android.widget.Button;
import android.widget.ImageView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.sound.TtsPlayer;
import com.mapswithme.maps.widget.RotateDrawable;
import com.mapswithme.util.Graphics;
import com.mapswithme.util.UiUtils;

public class NavMenu extends BaseMenu
{
  private final RotateDrawable mToggleImage;
  private final ImageView mTts;

  public enum Item implements BaseMenu.Item
  {
    TOGGLE(R.id.toggle),
    TTS_VOLUME(R.id.tts_volume),
    STOP(R.id.stop),
    SETTINGS(R.id.settings);

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

  public NavMenu(View frame, ItemClickListener<Item> listener)
  {
    super(frame, listener);

    mToggleImage = new RotateDrawable(Graphics.tint(mFrame.getContext(), R.drawable.ic_menu_close, R.attr.iconTintLight));
    ImageView toggle = (ImageView) mLineFrame.findViewById(R.id.toggle);
    toggle.setImageDrawable(mToggleImage);

    setToggleState(false, false);

    mapItem(Item.TOGGLE, mLineFrame);
    Button stop = (Button) mapItem(Item.STOP, mFrame);
    UiUtils.updateRedButton(stop);

    mapItem(Item.SETTINGS, mFrame);
    mTts = (ImageView) mapItem(Item.TTS_VOLUME, mFrame);
  }

  @Override
  public void onResume(@Nullable Runnable procAfterMeasurement)
  {
    super.onResume(procAfterMeasurement);
    refreshTts();
  }

  public void refreshTts()
  {
    mTts.setImageDrawable(TtsPlayer.isEnabled() ? Graphics.tint(mFrame.getContext(), R.drawable.ic_voice_on,
                                                                R.attr.colorAccent)
                                                : Graphics.tint(mFrame.getContext(), R.drawable.ic_voice_off));

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

    animator.setDuration(ANIMATION_DURATION);
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
  }
}