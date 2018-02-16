package com.mapswithme.maps.widget;

import android.animation.Animator;
import android.animation.AnimatorListenerAdapter;
import android.animation.ObjectAnimator;
import android.content.Context;
import android.graphics.drawable.Drawable;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.util.Graphics;
import com.mapswithme.util.UiUtils;

public class BookmarkBackupView extends LinearLayout
{
  private final static int ANIMATION_DURATION = 300;

  @SuppressWarnings("NullableProblems")
  @NonNull
  private View mHeader;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private View mContentLayout;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private TextView mTitle;

  @NonNull
  private final OnClickListener mHeaderClickListener = v -> onHeaderClick();

  private boolean mExpanded = true;

  public BookmarkBackupView(Context context)
  {
    super(context);
    init();
  }

  public BookmarkBackupView(Context context, @Nullable AttributeSet attrs)
  {
    super(context, attrs);
    init();
  }

  public BookmarkBackupView(Context context, @Nullable AttributeSet attrs, int defStyleAttr)
  {
    super(context, attrs, defStyleAttr);
    init();
  }

  private void init()
  {
    LayoutInflater.from(getContext()).inflate(R.layout.item_bookmark_backup, this);
    mHeader = findViewById(R.id.header);
    mContentLayout = findViewById(R.id.content);
    UiUtils.showIf(mExpanded, mContentLayout);

    mTitle = mHeader.findViewById(R.id.title);
    mTitle.setCompoundDrawablesWithIntrinsicBounds(null, null, getToggle(), null);
    mHeader.setOnClickListener(mHeaderClickListener);
  }

  private void onHeaderClick()
  {
    mExpanded = !mExpanded;
    Animator animator = mExpanded ? createFadeInAnimator() : createFadeOutAnimator();
    animator.setDuration(ANIMATION_DURATION);
    animator.start();
    mTitle.setCompoundDrawablesWithIntrinsicBounds(null, null, getToggle(), null);
  }

  @NonNull
  private Animator createFadeInAnimator()
  {
    ObjectAnimator animator = ObjectAnimator.ofFloat(mContentLayout, "alpha", 0, 1f);
    animator.addListener(new AnimatorListenerAdapter()
    {
      @Override
      public void onAnimationStart(Animator animation)
      {
        UiUtils.show(mContentLayout);
        mHeader.setEnabled(false);
      }

      @Override
      public void onAnimationEnd(Animator animation)
      {
        mHeader.setEnabled(true);
      }
    });
    return animator;
  }

  @NonNull
  private Animator createFadeOutAnimator()
  {
    ObjectAnimator animator = ObjectAnimator.ofFloat(mContentLayout, "alpha", 1f, 0);
    animator.addListener(new AnimatorListenerAdapter()
    {
      @Override
      public void onAnimationStart(Animator animation)
      {
        mHeader.setEnabled(false);
      }

      @Override
      public void onAnimationEnd(Animator animation)
      {
        UiUtils.hide(mContentLayout);
        mHeader.setEnabled(true);
      }
    });
    return animator;
  }

  @NonNull
  private Drawable getToggle()
  {
    return Graphics.tint(getContext(),
                         mExpanded ? R.drawable.ic_expand_less : R.drawable.ic_expand_more,
                         R.attr.secondary);
  }

  public void setMessage(@NonNull String msg)
  {
    TextView message = mContentLayout.findViewById(R.id.message);
    message.setText(msg);
  }

  public void setButtonLabel(@NonNull String label)
  {
    TextView button = mContentLayout.findViewById(R.id.button);
    button.setText(label);
  }

  public void hideButton()
  {
    UiUtils.hide(mContentLayout, R.id.button);
  }

  public void showButton()
  {
    UiUtils.show(mContentLayout, R.id.button);
  }

  public void hideProgressBar()
  {
    UiUtils.hide(mContentLayout, R.id.progress);
  }

  public void showProgressBar()
  {
    UiUtils.show(mContentLayout, R.id.progress);
  }

  public void setClickListener(@Nullable OnClickListener listener)
  {
    mContentLayout.findViewById(R.id.button).setOnClickListener(listener);
  }
}
