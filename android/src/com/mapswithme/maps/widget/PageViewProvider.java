package com.mapswithme.maps.widget;

import android.support.annotation.NonNull;
import android.view.View;

interface PageViewProvider
{
  @NonNull
  View findViewByIndex(int index);
}
