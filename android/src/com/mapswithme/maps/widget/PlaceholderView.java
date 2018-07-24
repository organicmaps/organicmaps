package com.mapswithme.maps.widget;

import android.annotation.TargetApi;
import android.content.Context;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.content.res.TypedArray;
import android.os.Build;
import android.support.annotation.DrawableRes;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.annotation.StringRes;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.util.UiUtils;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

public class PlaceholderView extends FrameLayout
{
  @Nullable
  private ImageView mImage;
  @Nullable
  private TextView mTitle;
  @Nullable
  private TextView mSubtitle;

  private int mImageSizeFull;
  private int mImageSizeSmall;
  private int mPaddingImage;
  private int mPaddingNoImage;

  @DrawableRes
  private int mImgSrcDefault;
  @StringRes
  private int mTitleResIdDefault;
  @StringRes
  private int mSubtitleResIdDefault;

  @NonNull
  private List<View> mTextChildren = Collections.emptyList();

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
    mImageSizeFull = res.getDimensionPixelSize(R.dimen.placeholder_size);
    mImageSizeSmall = res.getDimensionPixelSize(R.dimen.placeholder_size_small);
    mPaddingImage = res.getDimensionPixelSize(R.dimen.placeholder_margin_top);
    mPaddingNoImage = res.getDimensionPixelSize(R.dimen.placeholder_margin_top_no_image);
    LayoutInflater.from(context).inflate(R.layout.placeholder, this, true);

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

    mImage = findViewById(R.id.image);
    mTitle = findViewById(R.id.title);
    mSubtitle = findViewById(R.id.subtitle);

    List<View> textChildren = new ArrayList<>(getChildren());
    textChildren.remove(mImage);
    mTextChildren = Collections.unmodifiableList(textChildren);
    setupDefaultContent();
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
    if (isLandscape())
      measureLandscape(widthMeasureSpec, heightMeasureSpec);
    else
      measurePortrait(widthMeasureSpec, heightMeasureSpec);
  }

  private void measurePortrait(int widthMeasureSpec, int heightMeasureSpec)
  {
    measureChildren(widthMeasureSpec, heightMeasureSpec);
    boolean isImageSpaceAllowed = calcTotalChildrenHeight() > calcTotalTextChildrenHeight() + mImageSizeFull;
    UiUtils.showIf(isImageSpaceAllowed, mImage);

    if (mImage.getVisibility() == GONE)
      measureChildren(widthMeasureSpec, heightMeasureSpec);
    int topPadding = isImageSpaceAllowed ? mPaddingImage : mPaddingNoImage;

    int width = getDefaultSize(getSuggestedMinimumWidth(), widthMeasureSpec);
    int measuredWidth = width + getPaddingLeft() + getPaddingRight();
    int totalChildrenHeight = calcTotalChildrenHeight();
    int height = isImageSpaceAllowed
                 ? totalChildrenHeight
                 : totalChildrenHeight - calcHeightWithMargins(mImage);
    int measuredHeight = height + topPadding + getPaddingBottom();
    setMeasuredDimension(measuredWidth, measuredHeight);
  }

  private void measureLandscape(int widthMeasureSpec, int heightMeasureSpec)
  {
    UiUtils.hide(mImage);
    measureChildren(widthMeasureSpec, heightMeasureSpec);
    int width = getDefaultSize(getSuggestedMinimumWidth(), widthMeasureSpec);
    int height = calcTotalChildrenHeight() + getPaddingTop() + getPaddingBottom();
    setMeasuredDimension(width, height);
  }

  private int calcTotalChildrenHeight()
  {
    int totalChildrenHeight = 0;
    for (int index = 0; index < getChildCount(); index++)
    {
      totalChildrenHeight += calcHeightWithMargins(getChildAt(index));
    }
    return totalChildrenHeight;
  }

  private int calcTotalTextChildrenHeight()
  {
    int totalTextChildrenHeight = 0;
    for (View each : mTextChildren)
    {
      totalTextChildrenHeight += calcHeightWithMargins(each);
    }
    return totalTextChildrenHeight;
  }

  private boolean isLandscape()
  {
    return getContext().getResources().getConfiguration().orientation == Configuration.ORIENTATION_LANDSCAPE
           && !UiUtils.isTablet();
  }

  private int calcHeightWithMargins(@NonNull View view) {
    MarginLayoutParams params = (MarginLayoutParams) view.getLayoutParams();
    return view.getMeasuredHeight() + params.bottomMargin + params.topMargin;
  }

  @NonNull
  private List<View> getChildren()
  {
    return Collections.unmodifiableList(Arrays.asList(mImage , mTitle, mSubtitle));
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
