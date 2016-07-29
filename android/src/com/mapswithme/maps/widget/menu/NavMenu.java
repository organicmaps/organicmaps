package com.mapswithme.maps.widget.menu;

import android.animation.ValueAnimator;
import android.support.annotation.Nullable;
import android.view.View;
import android.widget.ImageView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.sound.TtsPlayer;
import com.mapswithme.maps.widget.RotateDrawable;

public class NavMenu extends BaseMenu
{
  private final ImageView mToggle;
  private final RotateDrawable mToggleImage = new RotateDrawable(R.drawable.ic_down);
  private final ImageView mTts;

  public enum Item implements BaseMenu.Item
  {
    TOGGLE(R.id.toggle),
    TTS_VOLUME(R.id.tts_volume),
    STOP(R.id.stop),
    //OVERVIEW(R.id.), TODO
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

    mToggle = (ImageView) mLineFrame.findViewById(R.id.toggle);
    //    mToggle.setImageDrawable(mToggleImage);
    mToggle.setOnClickListener(new View.OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        toggle(true);
      }
    });

    //    setToggleState(false, false);

    mapItem(Item.STOP, mFrame);
    mapItem(Item.SETTINGS, mFrame);
    mTts = (ImageView) mapItem(Item.TTS_VOLUME, mFrame);
  }

  @Override
  public void onResume(@Nullable Runnable procAfterCorrection)
  {
    super.onResume(procAfterCorrection);
    refreshTts();
  }

  public void refreshTts()
  {
    mTts.setImageResource(TtsPlayer.isEnabled() ? R.drawable.ic_voice_on
                                                : R.drawable.ic_voice_off);
  }

  @Override
  protected void setToggleState(boolean open, boolean animate)
  {
    if (!animate)
    {
      //      mToggleImage.setAngle(open ? -90.0f : 90.0f);
      return;
    }

    ValueAnimator animator = ValueAnimator.ofFloat(open ? 1.0f : 0, open ? 0 : 1.0f);
    animator.addUpdateListener(new ValueAnimator.AnimatorUpdateListener()
    {
      @Override
      public void onAnimationUpdate(ValueAnimator animation)
      {
        float fraction = (float) animation.getAnimatedValue();
        mToggleImage.setAngle((1.0f - fraction) * 180.0f);
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
}