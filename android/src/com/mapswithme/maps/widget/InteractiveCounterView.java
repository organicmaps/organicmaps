package com.mapswithme.maps.widget;

import android.content.Context;
import android.content.res.TypedArray;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.ImageView;
import android.widget.RelativeLayout;
import android.widget.TextView;

import androidx.annotation.DrawableRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.mapswithme.maps.R;
import com.mapswithme.util.UiUtils;

public class InteractiveCounterView extends RelativeLayout
{
  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private View mMinusView;
  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private View mPlusView;
  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private TextView mCounterView;
  @Nullable
  private String mTitle;
  @Nullable
  private String mSubtitle;
  private int mMinValue;
  private int mMaxValue;
  private int mDefaultValue;
  @DrawableRes
  private int mIconRes;
  @Nullable
  private CounterChangeListener mChangeListener;
  @NonNull
  private final OnClickListener mPlusClickListener = new OnClickListener()
  {
    @Override
    public void onClick(View v)
    {
      int value = getCurrentValue();
      mCounterView.setText(String.valueOf(++value));
      updateConstraints();
      if (mChangeListener != null)
        mChangeListener.onChange();
    }
  };

  @NonNull
  private final OnClickListener mMinusClickListener = new OnClickListener()
  {
    @Override
    public void onClick(View v)
    {
      int value = getCurrentValue();
      mCounterView.setText(String.valueOf(--value));
      updateConstraints();
      if (mChangeListener != null)
        mChangeListener.onChange();
    }
  };

  public InteractiveCounterView(Context context)
  {
    super(context);
  }

  public InteractiveCounterView(Context context, AttributeSet attrs)
  {
    super(context, attrs);
    init(attrs);
  }

  public InteractiveCounterView(Context context, AttributeSet attrs, int defStyleAttr)
  {
    super(context, attrs, defStyleAttr);
    init(attrs);
  }

  public InteractiveCounterView(Context context, AttributeSet attrs, int defStyleAttr, int defStyleRes)
  {
    super(context, attrs, defStyleAttr, defStyleRes);
    init(attrs);
  }

  private void init(@Nullable AttributeSet attrs)
  {
    TypedArray a = getContext().obtainStyledAttributes(attrs, R.styleable.InteractiveCounterView);
    try
    {
      mMinValue = a.getInt(R.styleable.InteractiveCounterView_minValue, 0);
      mMaxValue = a.getInt(R.styleable.InteractiveCounterView_maxValue, Integer.MAX_VALUE);
      mDefaultValue = a.getInt(R.styleable.InteractiveCounterView_android_defaultValue, mMinValue);
      mIconRes = a.getResourceId(R.styleable.InteractiveCounterView_android_src, UiUtils.NO_ID);
      mTitle = a.getString(R.styleable.InteractiveCounterView_title);
      mSubtitle = a.getString(R.styleable.InteractiveCounterView_subtitle);
      LayoutInflater.from(getContext()).inflate(R.layout.interactive_counter, this, true);
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
    ImageView iconView = findViewById(R.id.icon);
    iconView.setImageResource(mIconRes);
    TextView titleView = findViewById(R.id.title);
    UiUtils.showIf(!TextUtils.isEmpty(mTitle), titleView);
    titleView.setText(mTitle);
    TextView subtitleView = findViewById(R.id.subtitle);
    UiUtils.showIf(!TextUtils.isEmpty(mSubtitle), subtitleView);
    subtitleView.setText(mSubtitle);
    mMinusView = findViewById(R.id.minus);
    mMinusView.setOnClickListener(mMinusClickListener);
    mPlusView = findViewById(R.id.plus);
    mPlusView.setOnClickListener(mPlusClickListener);
    mCounterView = findViewById(R.id.counter);
    if (mDefaultValue >= mMinValue && mDefaultValue <= mMaxValue)
      mCounterView.setText(String.valueOf(mDefaultValue));
    else
      mCounterView.setText(String.valueOf(mMinValue));
    updateConstraints();
  }

  public int getCurrentValue()
  {
    return Integer.parseInt(mCounterView.getText().toString());
  }

  public void setCurrentValue(int value)
  {
    mCounterView.setText(String.valueOf(value));
    updateConstraints();
  }

  private void updateConstraints()
  {
    int value = getCurrentValue();
    mMinusView.setEnabled(value != mMinValue);
    mPlusView.setEnabled(value != mMaxValue);
  }

  public void setChangeListener(@Nullable CounterChangeListener listener)
  {
    mChangeListener = listener;
  }

  public interface CounterChangeListener
  {
    void onChange();
  }
}
