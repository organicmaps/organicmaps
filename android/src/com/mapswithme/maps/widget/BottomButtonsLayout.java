package com.mapswithme.maps.widget;

import android.content.Context;
import android.graphics.drawable.AnimationDrawable;
import android.support.annotation.NonNull;
import android.support.v4.view.ViewCompat;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.AccelerateInterpolator;
import android.view.animation.LinearInterpolator;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.RelativeLayout;
import android.widget.TextView;

import com.mapswithme.country.ActiveCountryTree;
import com.mapswithme.maps.R;
import com.mapswithme.util.UiUtils;
import com.nineoldandroids.animation.Animator;
import com.nineoldandroids.animation.AnimatorSet;
import com.nineoldandroids.animation.ObjectAnimator;
import com.nineoldandroids.animation.ValueAnimator;

/**
 * Layout for bottom menu button & sliding menu(search, settings, etc)
 */
public class BottomButtonsLayout extends RelativeLayout
{
  private static final long BUTTONS_ANIM_DURATION = 30;
  private static final long BUTTONS_ANIM_DURATION_LONG = 35;
  private ImageView mBtnSearch;
  private ImageButton mBtnMenu;
  private View mLlBookmarks;
  private View mLlSearch;
  private View mLlDownloader;
  private View mLlShare;
  private View mLlSettings;
  private TextView mTvOutdatedCount;

  private AnimationDrawable mAnimMenu;
  private AnimationDrawable mAnimMenuReversed;
  private AnimatorSet mButtonsAnimation;

  public BottomButtonsLayout(Context context)
  {
    this(context, null, 0);
  }

  public BottomButtonsLayout(Context context, AttributeSet attrs)
  {
    this(context, attrs, 0);
  }

  public BottomButtonsLayout(Context context, AttributeSet attrs, int defStyleAttr)
  {
    super(context, attrs, defStyleAttr);
    LayoutInflater.from(getContext()).inflate(R.layout.map_bottom_buttons, this);
    initButtons();
  }

  @SuppressWarnings("deprecation")
  private void initButtons()
  {
    ViewGroup root = (ViewGroup) findViewById(R.id.map_bottom_buttons);
    mLlBookmarks = root.findViewById(R.id.ll__bookmarks);
    mBtnSearch = (ImageView) root.findViewById(R.id.btn__search);
    mLlSearch = root.findViewById(R.id.ll__search);
    mLlDownloader = root.findViewById(R.id.ll__download_maps);
    mTvOutdatedCount = (TextView) root.findViewById(R.id.tv__outdated_maps_counter);
    mLlShare = root.findViewById(R.id.ll__share);
    mLlSettings = root.findViewById(R.id.ll__settings);
    mBtnMenu = (ImageButton) root.findViewById(R.id.btn__open_menu);

    mAnimMenu = (AnimationDrawable) getResources().getDrawable(R.drawable.anim_menu);
    mAnimMenuReversed = (AnimationDrawable) getResources().getDrawable(R.drawable.anim_menu_reversed);
  }

  /**
   * Toggles state of layout - ie shows, if buttons are hidden and hides otherwise
   */
  public void toggle()
  {
    if (areButtonsVisible())
      slideButtonsOut();
    else
      slideButtonsIn();
  }

  /**
   * @return true, if buttons are opened
   */
  public boolean areButtonsVisible()
  {
    return mLlSearch.getVisibility() == View.VISIBLE;
  }

  /**
   * Hides buttons immediately
   */
  public void hideButtons()
  {
    UiUtils.invisible(mLlBookmarks, mLlDownloader, mLlSettings, mLlShare, mLlSearch);
    // IMPORTANT views after alpha animations with 'setFillAfter' on 2.3 can't become GONE, until clearAnimationAfterAlpha is called.
    UiUtils.clearAnimationAfterAlpha(mLlSearch, mLlBookmarks, mLlDownloader, mLlSettings, mLlShare);
    if (mButtonsAnimation != null && mButtonsAnimation.isRunning())
      mButtonsAnimation.end();

    mBtnMenu.setImageResource(R.drawable.btn_green_menu);
    mBtnMenu.setVisibility(View.VISIBLE);
  }

  /**
   * Show buttons immediately
   */
  public void showButtons()
  {
    UiUtils.show(mLlBookmarks, mLlDownloader, mLlSettings, mLlShare, mLlSearch);
    mBtnMenu.setVisibility(View.GONE);
    refreshOutdatedMapsCounter();
  }

  /**
   * Shows buttons with animation
   */
  public void slideButtonsIn()
  {
    playMenuButtonAnimation();
    mBtnMenu.setVisibility(View.GONE);
    refreshOutdatedMapsCounter();
    mButtonsAnimation = new AnimatorSet();
    mLlSearch.setVisibility(View.VISIBLE);
    final float baseY = ViewCompat.getY(mLlSearch);
    mButtonsAnimation.play(generateMenuAnimator(mLlBookmarks, baseY - ViewCompat.getY(mLlBookmarks)));
    mButtonsAnimation.play(generateMenuAnimator(mLlDownloader, baseY - ViewCompat.getY(mLlDownloader)));
    mButtonsAnimation.play(generateMenuAnimator(mLlSettings, baseY - ViewCompat.getY(mLlSettings)));
    mButtonsAnimation.play(generateMenuAnimator(mLlShare, baseY - ViewCompat.getY(mLlShare)));
    mButtonsAnimation.addListener(new UiUtils.SimpleNineoldAnimationListener()
    {
      @Override
      public void onAnimationEnd(Animator animation)
      {
        mBtnMenu.setVisibility(View.GONE);
      }
    });
    mButtonsAnimation.start();
  }

  /**
   * Hides buttons with animation
   */
  public void slideButtonsOut()
  {
    hideButtons();
    playReverseMenuButtonAnimation();
  }

  private Animator generateMenuAnimator(@NonNull final View layout, final float translationY)
  {
    final float durationMultiplier = translationY / layout.getHeight();
    final AnimatorSet result = new AnimatorSet();
    ValueAnimator animator = ObjectAnimator.ofFloat(layout, "translationY", translationY, 0);
    animator.addListener(new UiUtils.SimpleNineoldAnimationListener()
    {
      @Override
      public void onAnimationStart(Animator animation)
      {
        layout.setVisibility(View.VISIBLE);
        ViewCompat.setAlpha(layout, 0);
      }
    });
    animator.setInterpolator(new LinearInterpolator());
    animator.setDuration((long) (BUTTONS_ANIM_DURATION * durationMultiplier));
    result.play(animator);

    animator = ObjectAnimator.ofFloat(layout, "alpha", 0, 1);
    animator.setDuration((long) (BUTTONS_ANIM_DURATION_LONG * durationMultiplier));
    animator.setInterpolator(new AccelerateInterpolator());
    result.play(animator);

    return result;
  }

  private void refreshOutdatedMapsCounter()
  {
    final int count = ActiveCountryTree.getOutOfDateCount();
    if (count == 0)
      mTvOutdatedCount.setVisibility(View.GONE);
    else
      UiUtils.setTextAndShow(mTvOutdatedCount, String.valueOf(count));
  }

  private void playMenuButtonAnimation()
  {
    mAnimMenu.selectDrawable(0);
    mAnimMenu.stop();
    mBtnSearch.setImageDrawable(mAnimMenu);
    mAnimMenu.start();
  }

  private void playReverseMenuButtonAnimation()
  {
    mAnimMenuReversed.selectDrawable(0);
    mAnimMenuReversed.stop();
    mBtnMenu.setImageDrawable(mAnimMenuReversed);
    mAnimMenuReversed.start();
  }
}
