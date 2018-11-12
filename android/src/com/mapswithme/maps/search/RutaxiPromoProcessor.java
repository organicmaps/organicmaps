package com.mapswithme.maps.search;

import android.app.Activity;
import android.support.annotation.NonNull;

import com.mapswithme.util.Utils;

public class RutaxiPromoProcessor implements PromoCategoryProcessor
{
  @NonNull
  private final Activity mActivity;

  RutaxiPromoProcessor(@NonNull Activity activity)
  {
    mActivity = activity;
  }

  @Override
  public void process()
  {
    Utils.openUrl(mActivity, "https://go.onelink.me/2944814706/86db6339");
  }
}
