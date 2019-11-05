package com.mapswithme.maps.widget;

import android.content.Context;
import android.content.res.ColorStateList;
import android.content.res.TypedArray;
import android.graphics.PorterDuff;
import android.graphics.drawable.Drawable;
import android.os.Build;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.FrameLayout;
import android.widget.ProgressBar;
import android.widget.TextView;

import androidx.annotation.ColorInt;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.RequiresApi;
import com.mapswithme.maps.R;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;

public class SubscriptionButton extends FrameLayout
{
  @Nullable
  private Drawable mButtonBackground;
  @ColorInt
  private int mButtonTextColor;
  @Nullable
  private Drawable mSaleBackground;
  @ColorInt
  private int mSaleTextColor;
  @ColorInt
  private int mProgressColor;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private TextView mSaleView;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private TextView mNameView;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private TextView mPriceView;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private ProgressBar mProgressBar;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private View mButtonContainer;
  private boolean mShowSale;

  public SubscriptionButton(@NonNull Context context)
  {
    super(context);
  }

  public SubscriptionButton(@NonNull Context context, @Nullable AttributeSet attrs)
  {
    super(context, attrs);
    init(attrs);
  }

  public SubscriptionButton(@NonNull Context context, @Nullable AttributeSet attrs,
                            int defStyleAttr)
  {
    super(context, attrs, defStyleAttr);
    init(attrs);
  }

  @RequiresApi(api = Build.VERSION_CODES.LOLLIPOP)
  public SubscriptionButton(@NonNull Context context, @Nullable AttributeSet attrs,
                            int defStyleAttr, int defStyleRes)
  {
    super(context, attrs, defStyleAttr, defStyleRes);
    init(attrs);
  }

  private void init(@Nullable AttributeSet attrs)
  {
    TypedArray a = getContext().obtainStyledAttributes(attrs, R.styleable.SubscriptionButton);
    try
    {
      mButtonBackground = a.getDrawable(R.styleable.SubscriptionButton_buttonBackground);
      mButtonTextColor = a.getColor(R.styleable.SubscriptionButton_buttonTextColor, 0);
      mProgressColor = a.getColor(R.styleable.SubscriptionButton_progressColor, 0);
      mSaleBackground = a.getDrawable(R.styleable.SubscriptionButton_saleBackground);
      mSaleTextColor = a.getColor(R.styleable.SubscriptionButton_saleTextColor, 0);
      mShowSale = a.getBoolean(R.styleable.SubscriptionButton_showSale, false);
      LayoutInflater.from(getContext()).inflate(R.layout.subscription_button, this, true);
    }
    finally
    {
      a.recycle();
    }
  }

  @Override
  protected void onFinishInflate()
  {
    super.onFinishInflate();
    mButtonContainer = findViewById(R.id.button_container);
    mButtonContainer.setBackground(mButtonBackground);
    mNameView = mButtonContainer.findViewById(R.id.name);
    mNameView.setTextColor(mButtonTextColor);
    mPriceView = mButtonContainer.findViewById(R.id.price);
    mPriceView.setTextColor(mButtonTextColor);
    mProgressBar = mButtonContainer.findViewById(R.id.progress);
    setProgressBarColor();
    mSaleView = findViewById(R.id.sale);
    if (mShowSale)
    {
      UiUtils.show(mSaleView);
      mSaleView.setBackground(mSaleBackground);
      mSaleView.setTextColor(mSaleTextColor);
    }
    else
    {
      UiUtils.hide(mSaleView);
      MarginLayoutParams params
          = (MarginLayoutParams) mButtonContainer.getLayoutParams();
      params.topMargin = 0;
    }
  }

  private void setProgressBarColor()
  {
    if (Utils.isLollipopOrLater())
      mProgressBar.setIndeterminateTintList(ColorStateList.valueOf(mProgressColor));
    else
      mProgressBar.getIndeterminateDrawable().setColorFilter(mProgressColor, PorterDuff.Mode.SRC_IN);
  }

  public void setName(@NonNull String name)
  {
    mNameView.setText(name);
  }

  public void setPrice(@NonNull String price)
  {
    mPriceView.setText(price);
  }

  public void setSale(@NonNull String sale)
  {
    mSaleView.setText(sale);
  }

  public void showProgress()
  {
    UiUtils.hide(mNameView, mPriceView);
    UiUtils.show(mProgressBar);
  }

  public void hideProgress()
  {
    UiUtils.hide(mProgressBar);
    UiUtils.show(mNameView, mPriceView);
  }

  @Override
  public void setOnClickListener(@Nullable OnClickListener listener)
  {
    mButtonContainer.setOnClickListener(listener);
  }
}
