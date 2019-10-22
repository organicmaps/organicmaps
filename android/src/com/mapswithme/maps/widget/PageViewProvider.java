package com.mapswithme.maps.widget;

import androidx.annotation.NonNull;
import android.view.View;

interface PageViewProvider
{
  @NonNull
  View findViewByIndex(int index);
}
