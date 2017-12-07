package com.mapswithme.maps.gallery;

import android.support.annotation.IntDef;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

public class Constants
{
  public static final int TYPE_PRODUCT = 0;
  public static final int TYPE_MORE = 1;
  @Retention(RetentionPolicy.SOURCE)
  @IntDef({ TYPE_PRODUCT, TYPE_MORE })
  @interface ViewType{}
}
