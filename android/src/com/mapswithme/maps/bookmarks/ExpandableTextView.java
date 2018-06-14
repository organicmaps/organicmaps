/*
 * Copyright (C) 2011 The Android Open Source Project
 * Copyright 2014 Manabu Shimobe
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.mapswithme.maps.bookmarks;

import android.annotation.TargetApi;
import android.content.Context;
import android.content.res.Resources;
import android.graphics.drawable.Drawable;
import android.os.Build;
import android.support.annotation.DrawableRes;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.util.SparseBooleanArray;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.AlphaAnimation;
import android.view.animation.Animation;
import android.view.animation.Transformation;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.mapswithme.maps.R;

public class ExpandableTextView extends LinearLayout implements View.OnClickListener {

  private static final String TAG = ExpandableTextView.class.getSimpleName();

  /* The default number of lines */
  private static final int MAX_COLLAPSED_LINES = 4;

  /* The default animation duration */
  private static final int DEFAULT_ANIM_DURATION = 300;

  /* The default alpha value when the animation starts */
  private static final float DEFAULT_ANIM_ALPHA_START = 0.7f;

  protected TextView mTv;

  protected View mButton; // Button to expand/collapse

  private boolean mRelayout;

  private boolean mCollapsed = true; // Show short version as default.

  private int mCollapsedHeight;

  private int mTextHeightWithMaxLines;

  private int mMaxCollapsedLines;

  private int mMarginBetweenTxtAndBottom;

  private Drawable mExpandDrawable;

  private Drawable mCollapseDrawable;

  private int mAnimationDuration;

  private float mAnimAlphaStart;

  private boolean mAnimating;

  /* Listener for callback */
  private OnExpandStateChangeListener mListener;

  /* For saving collapsed status when used in ListView */
  private SparseBooleanArray mCollapsedStatus;
  private int mPosition;
  @Nullable
  private OnClickListener mBtnClickListener;
  private boolean mForceCollapsingRestricted;

  public ExpandableTextView(Context context) {
    this(context, null);
  }

  public ExpandableTextView(Context context, AttributeSet attrs) {
    super(context, attrs);
    init(attrs);
  }

  @TargetApi(Build.VERSION_CODES.HONEYCOMB)
  public ExpandableTextView(Context context, AttributeSet attrs, int defStyle) {
    super(context, attrs, defStyle);
    init(attrs);
  }

  @Override
  public void setOrientation(int orientation){
    if(LinearLayout.HORIZONTAL == orientation){
      throw new IllegalArgumentException("ExpandableTextView only supports Vertical Orientation.");
    }
    super.setOrientation(orientation);
  }

  @Override
  public void onClick(View view) {
    if (mButton.getVisibility() != View.VISIBLE) {
      return;
    }

    if (mBtnClickListener != null) {
      mBtnClickListener.onClick(view);
      mMarginBetweenTxtAndBottom = getHeight() - getRealTextViewHeight(mTv);
      mTextHeightWithMaxLines = getRealTextViewHeight(mTv);
    }

    mForceCollapsingRestricted = true;
    mCollapsed = !mCollapsed;
//    mButton.setImageDrawable(mCollapsed ? mExpandDrawable : mCollapseDrawable);

    if (mCollapsedStatus != null) {
      mCollapsedStatus.put(mPosition, mCollapsed);
    }

    // mark that the animation is in progress
    mAnimating = true;

    Animation animation;
    if (mCollapsed) {
      animation = new ExpandCollapseAnimation(this, getHeight(), mCollapsedHeight);
    } else {
      animation = new ExpandCollapseAnimation(this, getHeight(), getHeight() +
                                                                 mTextHeightWithMaxLines - mTv.getHeight());
    }

    animation.setFillAfter(true);
    animation.setAnimationListener(new Animation.AnimationListener() {
      @Override
      public void onAnimationStart(Animation animation) {
        applyAlphaAnimation(mTv, mAnimAlphaStart);
      }
      @Override
      public void onAnimationEnd(Animation animation) {
        // clear animation here to avoid repeated applyTransformation() calls
        clearAnimation();
        // clear the animation flag
        mAnimating = false;

        // notify the listener
        if (mListener != null) {
          mListener.onExpandStateChanged(mTv, !mCollapsed);
        }
        mButton.setVisibility(mCollapsed ? VISIBLE : GONE);
      }
      @Override
      public void onAnimationRepeat(Animation animation) { }
    });

    clearAnimation();
    startAnimation(animation);
  }

  @Override
  public boolean onInterceptTouchEvent(MotionEvent ev) {
    // while an animation is in progress, intercept all the touch events to children to
    // prevent extra clicks during the animation
    return mAnimating;
  }

  @Override
  protected void onFinishInflate()
  {
    super.onFinishInflate();
    findViews();
  }

  @Override
  protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
    // If no change, measure and return
    if (!mRelayout || getVisibility() == View.GONE) {
      super.onMeasure(widthMeasureSpec, heightMeasureSpec);
      return;
    }
    mRelayout = false;

    // Setup with optimistic case
    // i.e. Everything fits. No button needed
    mButton.setVisibility(View.GONE);
    mTv.setMaxLines(Integer.MAX_VALUE);

    // Measure
    super.onMeasure(widthMeasureSpec, heightMeasureSpec);

    // If the text fits in collapsed mode, we are done.
    if (mTv.getLineCount() <= mMaxCollapsedLines && mForceCollapsingRestricted) {
      return;
    }

    // Saves the text height w/ max lines
    mTextHeightWithMaxLines = getRealTextViewHeight(mTv);

    // Doesn't fit in collapsed mode. Collapse text view as needed. Show
    // button.
    if (mCollapsed) {
      mTv.setMaxLines(mMaxCollapsedLines);
    }
    mButton.setVisibility(View.VISIBLE);

    // Re-measure with new setup
    super.onMeasure(widthMeasureSpec, heightMeasureSpec);

    if (mCollapsed) {
      // Gets the margin between the TextView's bottom and the ViewGroup's bottom
      mTv.post(new Runnable() {
        @Override
        public void run() {
          mMarginBetweenTxtAndBottom = getHeight() - mTv.getHeight();
        }
      });
      // Saves the collapsed height of this ViewGroup
      mCollapsedHeight = getMeasuredHeight();
    }
  }

  public void setOnExpandStateChangeListener(@Nullable OnExpandStateChangeListener listener) {
    mListener = listener;
  }

  public void setOnButtonClickListener(@NonNull OnClickListener clickListener) {
    mBtnClickListener = clickListener;
  }

  public void setText(@Nullable CharSequence text) {
    mRelayout = true;
    mTv.setText(text);
    setVisibility(TextUtils.isEmpty(text) ? View.GONE : View.VISIBLE);
  }

  public void setText(@Nullable CharSequence text, @NonNull SparseBooleanArray collapsedStatus, int position) {
    mCollapsedStatus = collapsedStatus;
    mPosition = position;
    boolean isCollapsed = collapsedStatus.get(position, true);
    clearAnimation();
    mCollapsed = isCollapsed;
//    mButton.setImageDrawable(mCollapsed ? mExpandDrawable : mCollapseDrawable);
    setText(text);
    getLayoutParams().height = ViewGroup.LayoutParams.WRAP_CONTENT;
    requestLayout();
  }

  @Nullable
  public CharSequence getText() {
    if (mTv == null) {
      return "";
    }
    return mTv.getText();
  }

  private void init(AttributeSet attrs) {
//    TypedArray typedArray = getContext().obtainStyledAttributes(attrs, R.styleable.ExpandableTextView);
    mMaxCollapsedLines = /*typedArray.getInt(R.styleable.ExpandableTextView_maxCollapsedLines,*/ MAX_COLLAPSED_LINES;
    mAnimationDuration = /*typedArray.getInt(R.styleable.ExpandableTextView_animDuration, */DEFAULT_ANIM_DURATION;
    mAnimAlphaStart = /*typedArray.getFloat(R.styleable.ExpandableTextView_animAlphaStart, */DEFAULT_ANIM_ALPHA_START/*)*/;
    mForceCollapsingRestricted = false;
/*    mExpandDrawable = typedArray.getDrawable(R.styleable.ExpandableTextView_expandDrawable*//*)*//*;
    mCollapseDrawable = typedArray.getDrawable(R.styleable.ExpandableTextView_collapseDrawable);*/

    if (mExpandDrawable == null) {
      mExpandDrawable = getDrawable(getContext(), R.drawable.ic_expand_small_holo_light);
    }
    if (mCollapseDrawable == null) {
      mCollapseDrawable = getDrawable(getContext(), R.drawable.ic_collapse_small_holo_light);
    }

//    typedArray.recycle();

    // enforces vertical orientation
    setOrientation(LinearLayout.VERTICAL);

    // default visibility is gone
    setVisibility(GONE);
  }

  private void findViews() {
    mTv = (TextView) findViewById(R.id.expandable_text);
    mTv.setOnClickListener(this);
    mButton = findViewById(R.id.expand_collapse);
//    mButton.setImageDrawable(mCollapsed ? mExpandDrawable : mCollapseDrawable);
    mButton.setOnClickListener(this);
  }

  private static boolean isPostHoneycomb() {
    return Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB;
  }

  private static boolean isPostLolipop() {
    return Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP;
  }

  @TargetApi(Build.VERSION_CODES.HONEYCOMB)
  private static void applyAlphaAnimation(View view, float alpha) {
    if (isPostHoneycomb()) {
      view.setAlpha(alpha);
    } else {
      AlphaAnimation alphaAnimation = new AlphaAnimation(alpha, alpha);
      // make it instant
      alphaAnimation.setDuration(0);
      alphaAnimation.setFillAfter(true);
      view.startAnimation(alphaAnimation);
    }
  }

  @TargetApi(Build.VERSION_CODES.LOLLIPOP)
  private static Drawable getDrawable(@NonNull Context context, @DrawableRes int resId) {
    Resources resources = context.getResources();
    if (isPostLolipop()) {
      return resources.getDrawable(resId, context.getTheme());
    } else {
      return resources.getDrawable(resId);
    }
  }

  private static int getRealTextViewHeight(@NonNull TextView textView) {
    int textHeight = textView.getLayout().getLineTop(textView.getLineCount());
    int padding = textView.getCompoundPaddingTop() + textView.getCompoundPaddingBottom();
    return textHeight + padding;
  }

  class ExpandCollapseAnimation extends Animation {
    private final View mTargetView;
    private final int mStartHeight;
    private final int mEndHeight;

    public ExpandCollapseAnimation(View view, int startHeight, int endHeight) {
      mTargetView = view;
      mStartHeight = startHeight;
      mEndHeight = endHeight;
      setDuration(mAnimationDuration);
    }

    @Override
    protected void applyTransformation(float interpolatedTime, Transformation t) {
      final int newHeight = (int)((mEndHeight - mStartHeight) * interpolatedTime + mStartHeight);
      mTv.setMaxHeight(newHeight - mMarginBetweenTxtAndBottom);
      if (Float.compare(mAnimAlphaStart, 1.0f) != 0) {
        applyAlphaAnimation(mTv, mAnimAlphaStart + interpolatedTime * (1.0f - mAnimAlphaStart));
      }
      mTargetView.getLayoutParams().height = newHeight;
      mTargetView.requestLayout();
    }

    @Override
    public void initialize( int width, int height, int parentWidth, int parentHeight ) {
      super.initialize(width, height, parentWidth, parentHeight);
    }

    @Override
    public boolean willChangeBounds( ) {
      return true;
    }
  };

  public interface OnExpandStateChangeListener {
    /**
     * Called when the expand/collapse animation has been finished
     *
     * @param textView - TextView being expanded/collapsed
     * @param isExpanded - true if the TextView has been expanded
     */
    void onExpandStateChanged(TextView textView, boolean isExpanded);
  }
}
