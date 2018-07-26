package com.mapswithme.maps.widget;

import android.annotation.TargetApi;
import android.content.Context;
import android.content.res.Resources;
import android.content.res.TypedArray;
import android.os.Build;
import android.support.annotation.DrawableRes;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.annotation.StringRes;
import android.support.v4.widget.NestedScrollView;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewParent;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.util.UiUtils;

public class PlaceholderView extends LinearLayout
{
  @Nullable
  private ImageView mImage;
  @Nullable
  private TextView mTitle;
  @Nullable
  private TextView mSubtitle;

  private int mImgMaxHeight;
  private int mImgMinHeight;

  @DrawableRes
  private int mImgSrcDefault;
  @StringRes
  private int mTitleResIdDefault;
  @StringRes
  private int mSubtitleResIdDefault;

  public PlaceholderView(Context context)
  {
    this(context, null, 0);
  }

  public PlaceholderView(Context context, @Nullable AttributeSet attrs)
  {
    this(context, attrs, 0);
  }

  public PlaceholderView(Context context, @Nullable AttributeSet attrs, int defStyleAttr)
  {
    super(context, attrs, defStyleAttr);

    init(context, attrs);
  }

  @TargetApi(Build.VERSION_CODES.LOLLIPOP)
  public PlaceholderView(Context context, @Nullable AttributeSet attrs, int defStyleAttr,
                         int defStyleRes)
  {
    super(context, attrs, defStyleAttr, defStyleRes);
    init(context, attrs);
  }

  private void init(Context context, AttributeSet attrs)
  {
    Resources res = getResources();
    mImgMaxHeight = res.getDimensionPixelSize(R.dimen.placeholder_size);
    mImgMinHeight = res.getDimensionPixelSize(R.dimen.placeholder_size_small);
    LayoutInflater inflater = LayoutInflater.from(context);
    inflater.inflate(R.layout.placeholder_image, this, true);
    inflater.inflate(R.layout.placeholder_title, this, true);
    inflater.inflate(R.layout.placeholder_subtitle, this, true);

    setOrientation(VERTICAL);
    initDefaultValues(context, attrs);
  }

  private void initDefaultValues(Context context, AttributeSet attrs)
  {
    TypedArray attrsArray = null;
    try
    {
      attrsArray =
          context.getTheme().obtainStyledAttributes(attrs, R.styleable.PlaceholderView, 0, 0);
      mImgSrcDefault = attrsArray.getResourceId(
          R.styleable.PlaceholderView_imgSrcDefault,
          UiUtils.NO_ID);
      mTitleResIdDefault = attrsArray.getResourceId(
          R.styleable.PlaceholderView_titleDefault,
          UiUtils.NO_ID);
      mSubtitleResIdDefault = attrsArray.getResourceId(
          R.styleable.PlaceholderView_subTitleDefault,
          UiUtils.NO_ID);
    }
    finally
    {
      if (attrsArray != null)
        attrsArray.recycle();
    }
  }

  @Override
  protected void onFinishInflate()
  {
    super.onFinishInflate();

    checkParent();
    mImage = findViewById(R.id.image);
    mTitle = findViewById(R.id.title);
    mSubtitle = findViewById(R.id.subtitle);

    setupDefaultContent();
  }

  private void checkParent()
  {
    ViewParent current = getParent();
    while (current != null)
    {
      if (current instanceof NestedScrollView || current instanceof ScrollView)
        throw new IllegalStateException("PlaceholderView can not be attached to NestedScrollView or ScrollView");
      current = current.getParent();
    }
  }

  private void setupDefaultContent()
  {
    if (isDefaultValueValid(mImage, mImgSrcDefault))
    {
      mImage.setImageResource(mImgSrcDefault);
    }
    if (isDefaultValueValid(mTitle, mTitleResIdDefault))
    {
      mTitle.setText(mTitleResIdDefault);
    }
    if (isDefaultValueValid(mSubtitle, mSubtitleResIdDefault))
    {
      mSubtitle.setText(mSubtitleResIdDefault);
    }
  }

  private static boolean isDefaultValueValid(View view, int defaultResId)
  {
    return view != null && defaultResId != UiUtils.NO_ID;
  }

  @Override
  protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec)
  {
    UiUtils.show(mImage);
    int childrenTotalHeight = calcTotalTextChildrenHeight(widthMeasureSpec, heightMeasureSpec);

    final int defHeight = getDefaultSize(getSuggestedMinimumWidth(), heightMeasureSpec);
    final int defWidth = getDefaultSize(getSuggestedMinimumWidth(), widthMeasureSpec);
    boolean isImageSpaceAllowed = defHeight > childrenTotalHeight + mImgMaxHeight;
    UiUtils.showIf(isImageSpaceAllowed, mImage);

    measureChildWithMargins(mImage, widthMeasureSpec, 0, heightMeasureSpec, childrenTotalHeight);
    childrenTotalHeight += isImageSpaceAllowed ? calcHeightWithMargins(mImage) : 0;

    final int height = childrenTotalHeight + getPaddingTop() + getPaddingBottom();
    setMeasuredDimension(defWidth, height);
  }

  private int calcTotalTextChildrenHeight(int widthMeasureSpec, int heightMeasureSpec)
  {
    int totalHeight = 0;
    for (int index = 0; index < getChildCount(); index++)
    {
      View child = getChildAt(index);
      if (child.getVisibility() == VISIBLE && child != mImage)
      {
        measureChildWithMargins(child, widthMeasureSpec , 0, heightMeasureSpec, totalHeight);
        totalHeight += calcHeightWithMargins(child);
      }
    }
    return totalHeight;
  }

  private int calcHeightWithMargins(@NonNull View view) {
    MarginLayoutParams params = (MarginLayoutParams) view.getLayoutParams();
    return view.getMeasuredHeight() + params.bottomMargin + params.topMargin;
  }

  public void setContent(@DrawableRes int imageRes, @StringRes int titleRes,
                         @StringRes int subtitleRes)
  {
    if (mImage != null)
      mImage.setImageResource(imageRes);
    if (mTitle != null)
      mTitle.setText(titleRes);
    if (mSubtitle != null)
      mSubtitle.setText(subtitleRes);
  }
}
