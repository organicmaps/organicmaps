package com.mapswithme.maps.search;

import android.app.Activity;
import androidx.annotation.NonNull;

import com.mapswithme.maps.Framework;
import com.mapswithme.util.Utils;

public class MegafonPromoProcessor implements PromoCategoryProcessor
{
  @NonNull
  private final Activity mActivity;

  MegafonPromoProcessor(@NonNull Activity activity)
  {
    mActivity = activity;
  }

  @Override
  public void process()
  {
    Utils.openUrl(mActivity, Framework.nativeGetMegafonCategoryBannerUrl());
  }
}
