package com.mapswithme.maps.widget;

import android.view.View;

import androidx.annotation.Nullable;

interface PageViewProvider
{
  @Nullable
  View findViewByIndex(int index);
}
