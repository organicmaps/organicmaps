package com.mapswithme.maps.widget;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import android.view.View;

interface PageViewProvider
{
  @Nullable
  View findViewByIndex(int index);
}
