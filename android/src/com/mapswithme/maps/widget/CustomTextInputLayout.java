package com.mapswithme.maps.widget;

import android.content.Context;
import android.graphics.Canvas;
import androidx.annotation.Nullable;
import com.google.android.material.textfield.TextInputLayout;
import androidx.core.view.ViewCompat;
import android.text.TextUtils;
import android.util.AttributeSet;

/**
 * Fixes bug mentioned here https://code.google.com/p/android/issues/detail?id=175228
 */
public class CustomTextInputLayout extends TextInputLayout
{
  private boolean mHintChanged = true;

  public CustomTextInputLayout(Context context)
  {
    super(context);
  }

  public CustomTextInputLayout(Context context, AttributeSet attrs)
  {
    super(context, attrs);
  }

  @Override
  protected void onDraw(Canvas canvas)
  {
    super.onDraw(canvas);

    if (mHintChanged && ViewCompat.isLaidOut(this))
    {
      // In case that hint is changed programmatically
      CharSequence currentEditTextHint = getEditText().getHint();
      if (!TextUtils.isEmpty(currentEditTextHint))
        setHint(currentEditTextHint);
      mHintChanged = false;
    }
  }

  @Override
  public void setHint(@Nullable CharSequence hint)
  {
    super.setHint(hint);
    mHintChanged = true;
  }
}
