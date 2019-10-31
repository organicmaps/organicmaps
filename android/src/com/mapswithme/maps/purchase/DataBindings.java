package com.mapswithme.maps.purchase;

import android.view.View;
import android.view.ViewGroup;

import androidx.databinding.BindingAdapter;

public class DataBindings
{
  @BindingAdapter("layout_marginTop_subs_button")
  public static void setTopMargin(View view, float margin) {
    ViewGroup.MarginLayoutParams layoutParams = (ViewGroup.MarginLayoutParams) view.getLayoutParams();
    layoutParams.topMargin = Math.round(margin);
    view.setLayoutParams(layoutParams);
  }
}
