package com.mapswithme.maps;

import android.view.View;
import android.widget.RelativeLayout;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

public class NavigationButtonsAnimationController
{
  @NonNull
  private final View mFrame;

  @Nullable
  private final OnTranslationChangedListener mTranslationListener;
  private float mContentHeight;
  private final float mBottomMargin;

  public NavigationButtonsAnimationController(@NonNull View frame,
                                              @Nullable OnTranslationChangedListener translationListener)
  {
    mFrame = frame;
    // Used to get the maximum height the buttons will evolve in
    View contentView = (View) mFrame.getParent();
    contentView.addOnLayoutChangeListener(new ContentViewLayoutChangeListener(contentView));
    mTranslationListener = translationListener;
    RelativeLayout.LayoutParams lp = (RelativeLayout.LayoutParams) frame.getLayoutParams();
    mBottomMargin = lp.bottomMargin;
  }

  public void move(float translationY)
  {
    if (mContentHeight == 0)
      return;

    final float translation = mBottomMargin + translationY - mContentHeight;
    update(translation <= 0 ? translation : 0);
  }

  public void update()
  {
    update(mFrame.getTranslationY());
  }

  private void update(final float translation)
  {
    mFrame.setTranslationY(translation);

    if (mTranslationListener != null)
      mTranslationListener.onTranslationChanged(translation);
  }

  public interface OnTranslationChangedListener
  {
    void onTranslationChanged(float translation);
  }

  private class ContentViewLayoutChangeListener implements View.OnLayoutChangeListener
  {
    @NonNull
    private final View mContentView;

    public ContentViewLayoutChangeListener(@NonNull View contentView)
    {
      mContentView = contentView;
    }

    @Override
    public void onLayoutChange(View v, int left, int top, int right, int bottom, int oldLeft,
                               int oldTop, int oldRight, int oldBottom)
    {
      mContentHeight = bottom - top;
      mContentView.removeOnLayoutChangeListener(this);
    }
  }
}
