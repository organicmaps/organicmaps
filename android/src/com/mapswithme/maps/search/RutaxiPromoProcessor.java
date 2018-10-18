package com.mapswithme.maps.search;

import android.content.Context;
import android.support.annotation.NonNull;

import com.mapswithme.util.Utils;

public class RutaxiPromoProcessor implements PromoCategoryProcessor
{
  @NonNull
  private final Context mContext;

  RutaxiPromoProcessor(@NonNull Context context)
  {
    mContext = context;
  }

  @Override
  public void process()
  {
    // TODO: add app launch when product decision about input params for vezet app is ready.
    Utils.openUrl(mContext, "https://go.onelink.me/757212956/a81b5d7c");
  }
}
